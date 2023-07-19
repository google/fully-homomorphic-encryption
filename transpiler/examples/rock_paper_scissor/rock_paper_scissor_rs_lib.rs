use tfhe::shortint::prelude::*;
use tfhe::shortint::CiphertextBig as Ciphertext;

// Encrypt a u8
pub fn encrypt(value: u8, client_key: &ClientKey) -> Vec<Ciphertext> {
    (0..8)
        .map(|shift| {
            let bit = (value >> shift) & 1;
            client_key.encrypt(if bit != 0 { 1 } else { 0 })
        })
        .collect()
}

// Decrypt a u8
pub fn decrypt(ciphertexts: &[Ciphertext], client_key: &ClientKey) -> u8 {
    let mut accum = 0u8;
    for (i, ct) in ciphertexts.iter().enumerate() {
        let bit = client_key.decrypt(ct);
        accum |= (bit as u8) << i;
    }
    accum
}

#[cfg(test)]
mod tests {
    use super::*;
    use rock_paper_scissor_rs_fhe_lib::rock_paper_scissor;
    use tfhe::shortint::parameters::PARAM_MESSAGE_1_CARRY_2;

    fn run_test_for(a: u8, b:u8) -> u8 {
        let (client_key, server_key) = gen_keys(PARAM_MESSAGE_1_CARRY_2);
        let fhe_a = encrypt(a, &client_key);
        let fhe_b = encrypt(b, &client_key);
        let fhe_output = &rock_paper_scissor(&fhe_a, &fhe_b, &server_key);
        decrypt(&fhe_output, &client_key)
    }

    #[test]
    fn test_rock_paper_scissor() {
        assert_eq!(run_test_for(b'R', b'R'), b'=');
        assert_eq!(run_test_for(b'R', b'P'), b'B');
        assert_eq!(run_test_for(b'S', b'P'), b'A');
    }
}
