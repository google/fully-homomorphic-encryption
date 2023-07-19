use tfhe::shortint::prelude::*;
use tfhe::shortint::CiphertextBig as Ciphertext;

// Encrypt an i8
pub fn encrypt(value: i8, client_key: &ClientKey) -> Vec<Ciphertext> {
    (0..8)
        .map(|shift| {
            let bit = (value >> shift) & 1;
            client_key.encrypt(if bit != 0 { 1 } else { 0 })
        })
        .collect()
}

// Decrypt an i8
pub fn decrypt(ciphertexts: &[Ciphertext], client_key: &ClientKey) -> i8 {
    let mut accum = 0i8;
    for (i, ct) in ciphertexts.iter().enumerate() {
        let bit = client_key.decrypt(ct);
        accum |= (bit as i8) << i;
    }
    accum
}

#[cfg(test)]
mod tests {
    use super::*;
    use ifte_rs_fhe_lib::ifte;
    use tfhe::shortint::parameters::PARAM_MESSAGE_1_CARRY_2;

    fn run_test_for(i: bool, t: i8, e: i8) -> i8 {
        let (client_key, server_key) = gen_keys(PARAM_MESSAGE_1_CARRY_2);
        // No support for single-bit inputs yet, so this example won't run
        let fhe_i = client_key.encrypt(i, &client_key);
        let fhe_t = encrypt(t, &client_key);
        let fhe_e = encrypt(e, &client_key);
        let fhe_output = &ifte(&fhe_i, &fhe_t, &fhe_e, &server_key);
        decrypt(&fhe_output, &client_key)
    }

    #[test]
    fn test_ifte() {
        assert_eq!(run_test_for(true, 1, 0), 1);
        assert_eq!(run_test_for(false, 1, 0), 0);
    }
}
