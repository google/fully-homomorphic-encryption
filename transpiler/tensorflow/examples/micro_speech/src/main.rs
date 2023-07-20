use std::time::Instant;
use tfhe::shortint::parameters::PARAM_MESSAGE_1_CARRY_2;
use tfhe::shortint::prelude::*;
use tfhe::shortint::CiphertextBig as Ciphertext;

mod micro_speech;

use crate::micro_speech::micro_speech_fhe_lib;
use crate::micro_speech::util::*;

fn main() {
    println!("Generating keys and encrypting input");
    let (client_key, server_key) = gen_keys(PARAM_MESSAGE_1_CARRY_2);
    let mut input: Vec<Ciphertext> =
        (0..1056).map(|_| encrypt_u8(1, &client_key)).flatten().collect();

    println!("Starting FHE computation");
    let start_computation = Instant::now();
    let output = micro_speech_fhe_lib::micro_speech(&input, &server_key);
    println!("Total computation took {:?} ms", start_computation.elapsed().as_millis());

    let mut output: Vec<u8> =
        output.chunks(8).map(|chunk| decrypt_u8(&chunk, &client_key)).collect();

    println!("Decrypted output: {:?}", output);
}
