use hangman_rs_fhe_lib::hangman_make_move;
use tfhe::shortint::CiphertextBig as Ciphertext;
use tfhe::shortint::parameters::PARAM_MESSAGE_1_CARRY_2;
use tfhe::shortint::prelude::*;

// Encrypt a u8
pub fn encrypt(value: u8, client_key: &ClientKey) -> Vec<Ciphertext> {
    (0..8)
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

fn run_test_for(x: u8) -> i32 {
    let (client_key, server_key) = gen_keys(PARAM_MESSAGE_1_CARRY_2);
    let fhe_val = encrypt(x, &client_key);
    let fhe_output = &hangman_make_move(&fhe_val, &server_key);
    decrypt(&fhe_output, &client_key)
}

#[cfg(not(test))]
fn main() {
    println!("Word: -------");
    println!("N? ");
    println!("N: {:07b}", run_test_for(b'n' as u8));
    println!("Word: --N---N");

    println!("G? ");
    println!("G: {:07b}", run_test_for(b'g' as u8));
    println!("Word: --NG--N");

    println!("A? ");
    println!("A: {:07b}", run_test_for(b'a' as u8));
    println!("Word: -ANG-AN");
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn test_hangman() {
        assert_eq!(run_test_for(b'h' as u8), 64);
        assert_eq!(run_test_for(b'n' as u8), 17);
        assert_eq!(run_test_for(b'x' as u8), 0);
    }
}
