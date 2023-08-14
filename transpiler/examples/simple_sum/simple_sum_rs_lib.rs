use tfhe::shortint::prelude::*;
use tfhe::shortint::CiphertextBig as Ciphertext;

// Encrypt a i32
pub fn encrypt(value: i32, client_key: &ClientKey) -> Vec<Ciphertext> {
    (0..32)
        .map(|shift| {
            let bit = (value >> shift) & 1;
            client_key.encrypt(if bit != 0 { 1 } else { 0 })
        })
        .collect()
}

// Decrypt a i32
pub fn decrypt(ciphertexts: &[Ciphertext], client_key: &ClientKey) -> i32 {
    let mut accum = 0i32;
    for (i, ct) in ciphertexts.iter().enumerate() {
        let bit = client_key.decrypt(ct);
        accum |= (bit as i32) << i;
    }
    accum
}

#[cfg(test)]
mod tests {
    use super::*;
    use simple_sum_rs_fhe_lib::simple_sum;
    use tfhe::shortint::parameters::PARAM_MESSAGE_1_CARRY_2;

    fn run_test_for(a: i32, b: i32) -> i32 {
        let (client_key, server_key) = gen_keys(PARAM_MESSAGE_1_CARRY_2);
        let mut fhe_a = encrypt(a, &client_key);
        let mut fhe_b = encrypt(b, &client_key);
        let fhe_output = &simple_sum(&mut fhe_a, &mut fhe_b, &server_key);
        decrypt(&fhe_output, &client_key)
    }

    #[test]
    fn test_simple_sum() {
        assert_eq!(run_test_for(123, 234), 357);
        assert_eq!(run_test_for(-5, 5), 0);
    }
}
