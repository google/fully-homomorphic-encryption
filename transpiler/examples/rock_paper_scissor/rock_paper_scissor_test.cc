#include "transpiler/examples/rock_paper_scissor/rock_paper_scissor.h"

#include "gtest/gtest.h"
#include "rock_paper_scissor.h"

namespace {

TEST(RockPaperScissorTest, WinA) {
  unsigned char result = rock_paper_scissor('P', 'R');

  EXPECT_EQ(result, 'A');
}

TEST(RockPaperScissorTest, Tie) {
  unsigned char result = rock_paper_scissor('P', 'P');

  EXPECT_EQ(result, '=');
}

TEST(RockPaperScissorTest, WinB) {
  unsigned char result = rock_paper_scissor('S', 'R');

  EXPECT_EQ(result, 'B');
}

}  // namespace
