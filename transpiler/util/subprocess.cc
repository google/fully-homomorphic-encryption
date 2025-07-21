// Copyright 2021 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "transpiler/util/subprocess.h"

#include <fcntl.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "absl/container/fixed_array.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"

namespace fully_homomorphic_encryption {
namespace transpiler {
namespace {

// Simple close-on-destruction FD holder.
class FileDescriptor {
 public:
  explicit FileDescriptor(int fd) : fd_(fd) {}
  ~FileDescriptor() { Close(); }

  FileDescriptor(const FileDescriptor&) = delete;
  FileDescriptor(FileDescriptor&& other) : fd_(other.fd_) { other.fd_ = -1; }
  FileDescriptor& operator=(FileDescriptor&& other) {
    Close();
    fd_ = other.fd_;
    other.fd_ = -1;
    return *this;
  }

  void Close() {
    if (fd_ != -1) {
      close(fd_);
      fd_ = -1;
    }
  }

  int get() const { return fd_; }

 private:
  int fd_ = -1;
};

struct Pipe {
  Pipe(FileDescriptor exit, FileDescriptor&& entrance)
      : exit(std::move(exit)), entrance(std::move(entrance)) {}

  // Opens a Unix pipe with a C++ friendly interface.
  static absl::StatusOr<Pipe> Open() {
    int descriptors[2];
    if (pipe2(descriptors, O_CLOEXEC) == -1) {
      return absl::InternalError(
          absl::StrCat("Failed to pipe: ", strerror(errno)));
    }
    return Pipe(FileDescriptor(descriptors[0]), FileDescriptor(descriptors[1]));
  }

  FileDescriptor exit;
  FileDescriptor entrance;
};

void PrepareAndExecInChildProcess(const std::vector<const char*>& argv_pointers,
                                  const std::filesystem::path& cwd,
                                  const Pipe& stdout_pipe,
                                  const Pipe& stderr_pipe) {
  if (!cwd.empty()) {
    if (chdir(cwd.c_str()) != 0) {
      std::cerr << "chdir failed: " << strerror(errno) << std::endl;
      _exit(127);
    }
  }

  while ((dup2(stdout_pipe.entrance.get(), STDOUT_FILENO) == -1) &&
         (errno == EINTR)) {
  }
  while ((dup2(stderr_pipe.entrance.get(), STDERR_FILENO) == -1) &&
         (errno == EINTR)) {
  }

  std::cerr << getpid() << ": Exec!" << std::endl;
  execv(argv_pointers[0], const_cast<char* const*>(argv_pointers.data()));
  std::cerr << "Execv syscall failed: " << strerror(errno) << std::endl;
  _exit(127);
}

// Takes a list of file descriptor data streams and reads them into a list of
// strings, one for each provided file descriptor. Uses poll.
absl::StatusOr<std::vector<std::string>> ReadFileDescriptors(
    absl::Span<FileDescriptor*> fds) {
  absl::FixedArray<char> buffer(4096);
  std::vector<std::string> result;
  result.resize(fds.size());
  std::vector<pollfd> poll_list;
  poll_list.resize(fds.size());
  for (int i = 0; i < fds.size(); i++) {
    poll_list[i].fd = fds[i]->get();
    poll_list[i].events = POLLIN;
  }
  int descriptors_left = fds.size();

  auto close_fd_by_index = [&](int idx) {
    poll_list[idx].fd = -1;
    descriptors_left--;
    fds[idx]->Close();
  };

  while (descriptors_left > 0) {
    int data_count = poll(poll_list.data(), poll_list.size(), -1);
    if (data_count <= 0) {
      if (errno == EINTR) {
        continue;
      }
      return absl::InternalError(absl::StrCat("poll failed:", strerror(errno)));
    }

    for (int i = 0; i < fds.size(); i++) {
      if (poll_list[i].revents & POLLERR) {
        // Unspecified error.
        return absl::InternalError("Subprocess poll failed.");
      }

      // This should "never" happen. If it does, someone has e.g. closed our fd.
      if (poll_list[i].revents & POLLNVAL) {
        std::cerr << "Poll failed - did someone close our fd???" << std::endl;
        exit(127);
      }

      // If poll_list[i].revents & POLLHUP, the remote side closed its
      // connection, but there may be data waiting to be read. read() will
      // return 0 bytes when we consume all the data, so just ignore that error.
      if ((poll_list[i].revents & (POLLHUP | POLLIN)) != 0) {
        int bytes = read(poll_list[i].fd, buffer.data(), buffer.size());
        if (bytes == 0) {
          // All data is read.
          close_fd_by_index(i);
        } else if (bytes > 0) {
          result[i].append(buffer.data(), bytes);
        } else if (errno != EINTR) {
          close_fd_by_index(i);
        }
      }
    }
  }

  return std::move(result);
}

// Waits for a process to finish. Returns its exit status code.
absl::StatusOr<int> WaitForPid(pid_t pid) {
  int wait_status;
  while (waitpid(pid, &wait_status, 0) == -1) {
    if (errno == EINTR) {
      continue;
    } else {
      return absl::InternalError(
          absl::StrCat("waitpid failed: ", strerror(errno)));
    }
  }
  return WEXITSTATUS(wait_status);
}

}  // namespace

absl::StatusOr<std::pair<std::string, std::string>> InvokeSubprocess(
    absl::Span<const std::string> argv, const std::filesystem::path& cwd) {
  if (argv.empty()) {
    return absl::InvalidArgumentError("Cannot invoke empty argv list.");
  }
  std::string bin_name = std::filesystem::path(argv[0]).filename();

  // Enable once we have a vlog-capable logging system.
#if 0
  VLOG(1) << absl::StreamFormat(
      "Running %s; argv: [ %s ], cwd: %s", bin_name, absl::StrJoin(argv, " "),
      cwd.string().empty() ? std::filesystem::current_path().string()
                           : cwd.string());
#endif

  std::vector<const char*> argv_pointers;
  argv_pointers.reserve(argv.size() + 1);
  for (const auto& arg : argv) {
    argv_pointers.push_back(arg.c_str());
  }
  argv_pointers.push_back(nullptr);

  // TODO: Status macros?
  auto error_or_pipe = Pipe::Open();
  if (!error_or_pipe.ok()) {
    return error_or_pipe.status();
  }
  Pipe stdout_pipe = std::move(error_or_pipe.value());

  error_or_pipe = Pipe::Open();
  if (!error_or_pipe.ok()) {
    return error_or_pipe.status();
  }
  Pipe stderr_pipe = std::move(error_or_pipe.value());

  pid_t pid = fork();
  if (pid == -1) {
    return absl::InternalError(
        absl::StrCat("Failed to fork: ", strerror(errno)));
  } else if (pid == 0) {
    PrepareAndExecInChildProcess(argv_pointers, cwd, stdout_pipe, stderr_pipe);
  }

  // This is the parent process.
  stdout_pipe.entrance.Close();
  stderr_pipe.entrance.Close();

  // Read from the output streams of the subprocess.
  FileDescriptor* fds[] = {&stdout_pipe.exit, &stderr_pipe.exit};
  auto status_or_output_strings = ReadFileDescriptors(fds);
  if (!status_or_output_strings.ok()) {
    return status_or_output_strings.status();
  }
  const auto& stdout_output = status_or_output_strings.value()[0];
  const auto& stderr_output = status_or_output_strings.value()[1];

  // Enable once we have a vlog-capable logging system.
#if 0
  VLOG_LINES(2, absl::StrCat(bin_name, " stdout:\n ", stdout_output));
  VLOG_LINES(2, absl::StrCat(bin_name, " stderr:\n ", stderr_output));
#endif

  // Wait for the subprocess to finish.
  auto exit_status = WaitForPid(pid).value();
  if (exit_status != 0) {
    return absl::InternalError(
        absl::StrFormat("Failed to execute %s; stdout: \"\"\"%s\"\"\"; "
                        "stderr: \"\"\"%s\"\"\"; exit code: %d",
                        bin_name, stdout_output, stderr_output, exit_status));
  }

  return std::make_pair(stdout_output, stderr_output);
}

}  // namespace transpiler
}  // namespace fully_homomorphic_encryption
