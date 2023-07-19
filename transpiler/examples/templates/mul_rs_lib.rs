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
    use mul_rs_fhe_lib::mul16;
    use tfhe::shortint::parameters::PARAM_MESSAGE_1_CARRY_2;

    fn run_test_for(a: i16, b:i16, c:i16) -> i16 {
        let (client_key, server_key) = gen_keys(PARAM_MESSAGE_1_CARRY_2);
        let fhe_a = encrypt(a, &client_key);
        let fhe_b = encrypt(b, &client_key);
        let fhe_c = encrypt(c, &client_key);
        let fhe_output = &mul16(&fhe_a, &fhe_b, &fhe_c, &server_key);
        decrypt(&fhe_output, &client_key)
    }

    #[test]
    fn test_mul() {
        assert_eq!(run_test_for(700, 15, 0), 10500);
        assert_eq!(run_test_for(-5, 5, 1), -24);
    }
}
