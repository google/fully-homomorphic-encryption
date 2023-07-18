google3::import! {
  "//third_party/fully_homomorphic_encryption/transpiler/examples/sqrt:sqrt_rs_fhe_lib" as sqrt_rs_fhe_lib;
}

use tfhe::shortint::prelude::*;
use tfhe::shortint::CiphertextBig as Ciphertext;

// Encrypt a i16
pub fn encrypt(value: i16, client_key: &ClientKey) -> Vec<Ciphertext> {
    (0..16)
        .map(|shift| {
            let bit = (value >> shift) & 1;
            client_key.encrypt(if bit != 0 { 1 } else { 0 })
        })
        .collect()
}

// Decrypt a i16
pub fn decrypt(ciphertexts: &[Ciphertext], client_key: &ClientKey) -> i16 {
    let mut accum = 0i16;
    for (i, ct) in ciphertexts.iter().enumerate() {
        let bit = client_key.decrypt(ct);
        accum |= (bit as i16) << i;
    }
    accum
}

#[cfg(test)]
mod tests {
    use super::*;
    use sqrt_rs_fhe_lib::isqrt;
    use tfhe::shortint::parameters::PARAM_MESSAGE_1_CARRY_2;

    fn run_test_for(x: i16) -> i16 {
        let (client_key, server_key) = gen_keys(PARAM_MESSAGE_1_CARRY_2);
        let fhe_val = encrypt(x, &client_key);
        let fhe_output = &isqrt(&fhe_val, &server_key);
        decrypt(&fhe_output, &client_key)
    }

    #[test]
    fn test_sqrt() {
        assert_eq!(run_test_for(169), 13);
    }
}
