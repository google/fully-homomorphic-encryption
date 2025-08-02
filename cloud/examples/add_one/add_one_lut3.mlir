module {
  func.func @add_one_lut3(%in: i8 {secret.secret}) -> (i8) {
    %1 = arith.constant 1 : i8
    %2 = arith.addi %in, %1 : i8
    return %2 : i8
  }
}
