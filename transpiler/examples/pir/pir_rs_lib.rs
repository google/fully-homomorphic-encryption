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
    use pir_rs_fhe_lib::query_record;
    use tfhe::shortint::parameters::PARAM_MESSAGE_1_CARRY_2;

    fn run_test_for(db: Vec<u8>, index: u8) -> u8 {
        let (client_key, server_key) = gen_keys(PARAM_MESSAGE_1_CARRY_2);
        let fhe_database = db.into_iter().flat_map(|item| encrypt(item, &client_key).into_iter()).collect();
        let fhe_index = encrypt(index, &client_key);
        let fhe_output = &query_record(&fhe_database, &fhe_index, &server_key);
        decrypt(&fhe_output, &client_key)
    }

    #[test]
    fn test_pir() {
        assert_eq!(run_test_for(vec![1, 2, 3], 1), 2);
        assert_eq!(run_test_for(vec![4, 1, 9], 2), 9);
    }
}
