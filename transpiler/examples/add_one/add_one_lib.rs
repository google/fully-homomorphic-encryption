use tfhe::shortint::prelude::*;
use tfhe::shortint::CiphertextBig as Ciphertext;

// Encrypt a u8
pub fn encrypt(value: u8, client_key: &ClientKey) -> Vec<Ciphertext> {
    (0..8)
        .map(|shift| {
            let bit = (value >> shift) & 1;
            return client_key.encrypt(if bit != 0 { 1 } else { 0 });
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
    #[cfg(lut)]
    use add_one_fhe_rs_lib::add_one;
    #[cfg(not(lut))]
    use add_one_gate_fhe_rs_lib::add_one;
    #[cfg(lut)]
    use tfhe::shortint::parameters::PARAM_MESSAGE_1_CARRY_2;

    fn run_test_for(x: u8) -> u8 {
        let (client_key, server_key) = gen_keys(PARAM_MESSAGE_1_CARRY_2);
        let fhe_val = encrypt(x, &client_key);
        decrypt(&add_one(&fhe_val, &server_key), &client_key)
    }

    #[test]
    fn test_add_one_zero() {
        assert_eq!(run_test_for(0), 1);
    }

    #[test]
    fn test_add_one_7() {
        assert_eq!(run_test_for(7), 8);
    }

    #[test]
    fn test_add_one_255() {
        assert_eq!(run_test_for(255), 0);
    }
}
