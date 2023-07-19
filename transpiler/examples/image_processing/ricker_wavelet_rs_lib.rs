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
    use ricker_wavelet_rs_fhe_lib::ricker_wavelet;
    use tfhe::shortint::parameters::PARAM_MESSAGE_1_CARRY_2;

    fn run_test_for(x: Vec<u8>) -> u8 {
        let (client_key, server_key) = gen_keys(PARAM_MESSAGE_1_CARRY_2);
        let fhe_val = x.into_iter().flat_map(|char| encrypt(char, &client_key).into_iter()).collect();
        let fhe_output = &ricker_wavelet(&fhe_val, &server_key);
        decrypt(&fhe_output, &client_key)
    }

    #[test]
    fn test_zeros() {
        assert_eq!(run_test_for(vec![0, 0, 0, 0, 0, 0, 0, 0, 0]), 0);
    }

    #[test]
    fn test_max_value() {
        assert_eq!(run_test_for(vec![0, 0, 0, 0, 255, 0, 0, 0, 0]), 255);
    }

    #[test]
    fn test_min_value_abs_reflects() {
        assert_eq!(run_test_for(vec![255, 255, 255, 255, 0, 255, 255, 255, 255]), 255);
    }

    #[test]
    fn test_random_values() {
        assert_eq!(run_test_for(vec![76, 35, 178, 140, 30, 205, 94, 219, 252]), 119);
    }
}
