# Fully Homomorphic Encryption (FHE)

**Note:** [HEIR](https://github.com/google/heir) is our next generation FHE
compiler framework, please see [its GitHub repo](https://github.com/google/heir)
and its website https://heir.dev.

This repository contains open-source libraries and tools to perform fully
homomorphic encryption (FHE) operations on an encrypted data set.

**About Fully Homomorphic Encryption**

Fully Homomorphic Encryption (FHE) is an emerging cryptographic technique that
allows developers to perform computations on encrypted data. This represents a
paradigm shift in how data processing and data privacy relate to each other.

Previously, if an application had to perform some computation on data that was
encrypted, this application would necessarily need to decrypt the data first,
perform the desired computations on the clear data, and then re-encrypt the
data. FHE, on the other hand, simply removes the need for this
decryption-encryption steps by the application, all at once.

In practice, for an application that needs to perform some computation F on data
that is encrypted, the FHE scheme would provide some alternative computation F'
which when applied directly over the encrypted data will result in the
encryption of the application of F over the data in the clear. More formally:
F(unencrypted_data) = Decrypt(F'(encrypted_data)).

As a result, FHE can have an enormous impact to our society. It can change the
way computations are performed by preserving end-to-end privacy. For example,
users would be able to offload expensive computations to cloud providers in a
way that cloud providers will not have access to the users' data at all.

The main hindrance for the adoption of FHE has been its very poor performance.
Despite significant scientific improvements, performing computations on
encrypted data using FHE is still orders of magnitude slower than performing the
computation on the plaintext. On top of that, converting a program that operates
on unencrypted data to one that FHE-operates on encrypted data is far from being
a trivial translation. If not properly done, this translation can significantly
increase the performance gap between computing on unencrypted data and the
FHE-computation on encrypted data, thus precluding wide FHE adoption.

## FHE C++ Transpiler

The FHE C++ Transpiler is a general purpose library that converts C++ into
FHE-C++ that works on encrypted input.

The transpiler has a modular architecture that allows varying the underlying FHE
library, the high-level program description and the output language as well. We
hope that this flexibility will allow researchers from different fields to work
together on this exciting goal of making FHE more efficient and broadly
applicable.

The code, examples, and more information is in the [`transpiler`](./transpiler/)
subdirectory.

## Support

We will continue to publish updates and improvements to the FHE library. We are
not yet accepting external contributions to this project. We will respond to
issues filed in this project. If we ever intend to stop publishing improvements
and responding to issues we will publish notice here at least 3 months in
advance.

## Support disclaimer

This is not an officially supported Google product.

## License

Apache License 2.0. See [`LICENSE`](./LICENSE).

## Contact information

We are committed to open-sourcing our work to support your use cases. We want to
know how you use this library and what problems it helps you to solve. We have
two communication channels for you to contact us:

*   A
    [public discussion group](https://groups.google.com/g/fhe-open-source-users)
    where we will also share our preliminary roadmap, updates, events, and more.

*   A private email alias at
    [fhe-open-source@google.com](mailto:fhe-open-source@google.com) where you
    can reach out to us directly about your use cases and what more we can do to
    help and improve the library.

Please refrain from sending any sensitive or confidential information. If you
wish to delete a message you've previously sent, please contact us.

## Contributors

The contributors to this project are (sorted by last name):

-   [Eric Astor](https://github.com/ericastor)
-   [Damien Desfontaines](https://desfontain.es/serious.html)
-   Christoph Dibak
-   [Alain Forget](https://people.scs.carleton.ca/~aforget/)
-   [Bryant Gipson](https://www.linkedin.com/in/bryant-gipson-33478419)
-   [Shruthi Gorantala](https://github.com/code-perspective) (Lead)
-   [Miguel Guevara](https://www.linkedin.com/in/miguel-guevara-8a5a332a)
-   [Aishwarya Krishnamurthy](https://www.linkedin.com/in/aishe-k)
-   Sasha Kulankhina
-   [William Lam](https://www.linkedin.com/in/william-m-lam)
-   [David Marn](http://dmarn.org)
-   [Rafael Misoczki](https://www.linkedin.com/in/rafael-misoczki-phd-24b33013)
-   Bernat Guillén Pegueroles
-   [Milinda Perera](https://milinda-perera.com)
-   [Sean Purser-Haskell](https://www.linkedin.com/in/sean-purser-haskell-30b5268)
-   [Sam Ruth](https://www.linkedin.com/in/samuelruth)
-   [Rob Springer](https://github.com/RobSpringer)
-   [Yurii Sushko](https://www.linkedin.com/in/midnighter)
-   [Cameron Tew](https://github.com/cam2337)
-   [Royce Wilson](https://research.google/people/RoyceJWilson)
-   [Xinyu Ye](https://github.com/xinyuye)
-   [Itai Zukerman](https://github.com/izuk)
-   [Iliyan Malchev](https://github.com/malchev)

## Citing FHE Transpiler

To cite FHE Transpiler in academic papers, please use the following entry:

```shell
@misc{cryptoeprint:2021/811,
      author = {Shruthi Gorantala and Rob Springer and Sean Purser-Haskell and William Lam and Royce Wilson and Asra Ali and Eric P. Astor and Itai Zukerman and Sam Ruth and Christoph Dibak and Phillipp Schoppmann and Sasha Kulankhina and Alain Forget and David Marn and Cameron Tew and Rafael Misoczki and Bernat Guillen and Xinyu Ye and Dennis Kraft and Damien Desfontaines and Aishe Krishnamurthy and Miguel Guevara and Irippuge Milinda Perera and Yurii Sushko and Bryant Gipson},
      title = {A General Purpose Transpiler for Fully Homomorphic Encryption},
      howpublished = {Cryptology ePrint Archive, Paper 2021/811},
      year = {2021},
      note = {\url{https://eprint.iacr.org/2021/811}},
      url = {https://eprint.iacr.org/2021/811}
}
```
