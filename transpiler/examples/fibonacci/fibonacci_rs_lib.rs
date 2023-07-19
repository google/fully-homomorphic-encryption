use fibonacci_rs_fhe_lib::fibonacci_number;
use tfhe::shortint::CiphertextBig as Ciphertext;
use tfhe::shortint::parameters::PARAM_MESSAGE_1_CARRY_2;
use tfhe::shortint::prelude::*;

// Encrypt an i32
pub fn encrypt(value: i32, client_key: &ClientKey) -> Vec<Ciphertext> {
    (0..32)
        .map(|shift| {
            let bit = (value >> shift) & 1;
            client_key.encrypt(if bit != 0 { 1 } else { 0 })
        })
        .collect()
}

// Decrypt an i32
pub fn decrypt(ciphertexts: &[Ciphertext], client_key: &ClientKey) -> i32 {
    let mut accum = 0i32;
    for (i, ct) in ciphertexts.iter().enumerate() {
        let bit = client_key.decrypt(ct);
        accum |= (bit as i32) << i;
    }
    accum
}

fn run_test_for(x: i32) -> i32 {
    let (client_key, server_key) = gen_keys(PARAM_MESSAGE_1_CARRY_2);
    let fhe_val = encrypt(x, &client_key);
    let fhe_output = &fibonacci_number(&fhe_val, &server_key);
    decrypt(&fhe_output, &client_key)
}

#[cfg(not(test))]
fn main() {
    for input in 0..8 {
        println!("Input: {:?}, output {:?}", input, run_test_for(input));
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_fibonacci() {
        assert_eq!(run_test_for(4), 3);
        assert_eq!(run_test_for(6), 8);
        assert_eq!(run_test_for(8), 21);
    }
}
