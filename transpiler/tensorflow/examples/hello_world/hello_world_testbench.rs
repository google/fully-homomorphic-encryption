use hello_world_fhe_lib;

use std::time::Instant;
use tfhe::shortint::prelude::*;
use tfhe::shortint::CiphertextBig as Ciphertext;

// Encrypt a i8
pub fn encrypt(value: i8, client_key: &ClientKey) -> Vec<Ciphertext> {
    (0..8)
        .map(|shift| {
            let bit = (value >> shift) & 1;
            client_key.encrypt(if bit != 0 { 1 } else { 0 })
        })
        .collect()
}

// Decrypt a i8
pub fn decrypt(ciphertexts: &[Ciphertext], client_key: &ClientKey) -> i8 {
    let mut accum = 0i8;
    for (i, ct) in ciphertexts.iter().enumerate() {
        let bit = client_key.decrypt(ct);
        accum |= (bit as i8) << i;
    }
    accum
}

fn main() {
    use hello_world_fhe_lib::hello_world;
    use tfhe::shortint::parameters::PARAM_MESSAGE_1_CARRY_2;

    // Quantize an input float according to model quantization.
    fn quantize(x: f32) -> i8 {
        return (x / 0.024480115622282 - 128.0) as i8;
    }

    // Dequantize the output integer according to model quantization.
    fn dequantize(x: i8) -> f32 {
        return ((x as f32) - 5.0) * 0.00829095672816038;
    }

    fn run_hello_world(x: i8, client_key: &ClientKey, server_key: &ServerKey) -> i8 {
        let fhe_in = encrypt(x, &client_key);
        let start = Instant::now();
        let fhe_out = &hello_world(&fhe_in, &server_key);
        let duration = start.elapsed();
        println!("FHE computation in {:?} s", duration.as_secs());
        decrypt(&fhe_out, &client_key)
    }

    let (client_key, server_key) = gen_keys(PARAM_MESSAGE_1_CARRY_2);
    let pi = 3.14159265359;
    let x_vals = vec![0.0, pi * 0.5, pi, pi * 1.5, pi * 2.0];
    for x in x_vals {
        println!("Inferring sine for {}", x);
        let quantize_x = quantize(x);
        let out = run_hello_world(quantize_x, &client_key, &server_key);
        let dequantize_y = dequantize(out);
        println!("Sine value: {:.2}", dequantize_y);
    }
}
