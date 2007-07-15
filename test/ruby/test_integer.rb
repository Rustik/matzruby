require 'test/unit'

class TestInteger < Test::Unit::TestCase
  VS = [
    -0x10000000000000002,
    -0x10000000000000001,
    -0x10000000000000000,
    -0xffffffffffffffff,
    -0x4000000000000002,
    -0x4000000000000001,
    -0x4000000000000000,
    -0x3fffffffffffffff,
    -0x100000002,
    -0x100000001,
    -0x100000000,
    -0xffffffff,
    -0xc717a08d, # 0xc717a08d * 0x524b2245 = 0x4000000000000001
    -0x80000002,
    -0x80000001,
    -0x80000000,
    -0x7fffffff,
    -0x524b2245,
    -0x40000002,
    -0x40000001,
    -0x40000000,
    -0x3fffffff,
    -0x10002,
    -0x10001,
    -0x10000,
    -0xffff,
    -0x8101, # 0x8101 * 0x7f01 = 0x40000001
    -0x8002,
    -0x8001,
    -0x8000,
    -0x7fff,
    -0x7f01,
    -65,
    -64,
    -63,
    -62,
    -33,
    -32,
    -31,
    -30,
    -3,
    -2,
    -1,
    0,
    1,
    2,
    3,
    30,
    31,
    32,
    33,
    62,
    63,
    64,
    65,
    0x7f01,
    0x7ffe,
    0x7fff,
    0x8000,
    0x8001,
    0x8101,
    0xfffe,
    0xffff,
    0x10000,
    0x10001,
    0x3ffffffe,
    0x3fffffff,
    0x40000000,
    0x40000001,
    0x524b2245,
    0x7ffffffe,
    0x7fffffff,
    0x80000000,
    0x80000001,
    0xc717a08d,
    0xfffffffe,
    0xffffffff,
    0x100000000,
    0x100000001,
    0x3ffffffffffffffe,
    0x3fffffffffffffff,
    0x4000000000000000,
    0x4000000000000001,
    0xfffffffffffffffe,
    0xffffffffffffffff,
    0x10000000000000000,
    0x10000000000000001,
  ]

  #VS.map! {|v| 0x4000000000000000.coerce(v)[0] }

  def test_aref
    VS.each {|a|
      100.times {|i|
        assert_equal((a >> i).odd? ? 1 : 0, a[i], "(#{a})[#{i}]")
      }
    }
  end

  def test_plus
    VS.each {|a|
      VS.each {|b|
        c = a + b
        assert_equal(b + a, c, "#{a} + #{b}")
        assert_equal(a, c - b, "(#{a} + #{b}) - #{b}")
        assert_equal(a-~b-1, c, "#{a} + #{b}") # Hacker's Delight
        assert_equal((a^b)+2*(a&b), c, "#{a} + #{b}") # Hacker's Delight
        assert_equal((a|b)+(a&b), c, "#{a} + #{b}") # Hacker's Delight
        assert_equal(2*(a|b)-(a^b), c, "#{a} + #{b}") # Hacker's Delight
      }
    }
  end

  def test_minus
    VS.each {|a|
      VS.each {|b|
        c = a - b
        assert_equal(a, c + b, "(#{a} - #{b}) + #{b}")
        assert_equal(-b, c - a, "(#{a} - #{b}) - #{a}")
        assert_equal(a+~b+1, c, "#{a} - #{b}") # Hacker's Delight
        assert_equal((a^b)-2*(b&~a), c, "#{a} - #{b}") # Hacker's Delight
        assert_equal((a&~b)-(b&~a), c, "#{a} - #{b}") # Hacker's Delight
        assert_equal(2*(a&~b)-(a^b), c, "#{a} - #{b}") # Hacker's Delight
      }
    }
  end

  def test_mult
    VS.each {|a|
      VS.each {|b|
        c = a * b
        assert_equal(b * a, c, "#{a} * #{b}")
        assert_equal(b, c / a, "(#{a} * #{b}) / #{a}") if a != 0
        assert_equal(a.abs * b.abs, (a * b).abs, "(#{a} * #{b}).abs")
        assert_equal((a-100)*(b-100)+(a-100)*100+(b-100)*100+10000, c, "#{a} * #{b}")
        assert_equal((a+100)*(b+100)-(a+100)*100-(b+100)*100+10000, c, "#{a} * #{b}")
      }
    }
  end

  def test_divmod
    VS.each {|a|
      VS.each {|b|
        next if b == 0
        q, r = a.divmod(b)
        assert_equal(a, b*q+r)
        assert(r.abs < b.abs)
        assert(0 < b ? (0 <= r && r < b) : (b < r && r <= 0))
        assert_equal(q, a/b)
        assert_equal(q, a.div(b))
        assert_equal(r, a%b)
        assert_equal(r, a.modulo(b))
      }
    }
  end

  def test_pow
    small_values = VS.find_all {|v| 0 <= v && v < 1000 }
    VS.each {|a|
      small_values.each {|b|
        c = a ** b
        d = 1
        b.times { d *= a }
        assert_equal(d, c, "(#{a}) ** #{b}")
        if a != 0
          d = c
          b.times { d /= a }
          assert_equal(1, d, "((#{a}) ** #{b}) / #{a} / ...(#{b} times)...")
        end
      }
    }
  end

  def test_not
    VS.each {|a|
      b = ~a
      assert_equal(-1 ^ a, b, "~#{a}")
      assert_equal(-a-1, b, "~#{a}") # Hacker's Delight
      assert_equal(0, a & b, "#{a} & ~#{a}")
      assert_equal(-1, a | b, "#{a} | ~#{a}")
    }
  end

  def test_or
    VS.each {|a|
      VS.each {|b|
        c = a | b
        assert_equal(b | a, c, "#{a} | #{b}")
        assert_equal(a + b - (a&b), c, "#{a} | #{b}")
        assert_equal((a & ~b) + b, c, "#{a} | #{b}") # Hacker's Delight
        assert_equal(-1, c | ~a, "(#{a} | #{b}) | ~#{a})")
      }
    }
  end

  def test_and
    VS.each {|a|
      VS.each {|b|
        c = a & b
        assert_equal(b & a, c, "#{a} & #{b}")
        assert_equal(a + b - (a|b), c, "#{a} & #{b}")
        assert_equal((~a | b) - ~a, c, "#{a} & #{b}") # Hacker's Delight
        assert_equal(0, c & ~a, "(#{a} & #{b}) & ~#{a}")
      }
    }
  end

  def test_xor
    VS.each {|a|
      VS.each {|b|
        c = a ^ b
        assert_equal(b ^ a, c, "#{a} ^ #{b}")
        assert_equal((a|b)-(a&b), c, "#{a} ^ #{b}") # Hacker's Delight
        assert_equal(b, c ^ a, "(#{a} ^ #{b}) ^ #{a}")
      }
    }
  end

  def test_lshift
    small_values = VS.find_all {|v| -1000 < v && v < 1000 }
    VS.each {|a|
      small_values.each {|b|
        c = a << b
        if 0 <= b
          assert_equal(a, c >> b, "(#{a} << #{b}) >> #{b}")
          assert_equal(a * 2**b, c, "#{a} << #{b}")
        else
          assert_equal(a / 2**(-b), c, "#{a} << #{b}")
        end
      }
    }
  end

  def test_rshift
    small_values = VS.find_all {|v| -1000 < v && v < 1000 }
    VS.each {|a|
      small_values.each {|b|
        c = a >> b
        if 0 < b
          assert_equal(a / 2**b, c, "#{a} >> #{b}")
        else
          assert_equal(a, c << b, "(#{a} >> #{b}) << #{b}")
          assert_equal(a * 2**(-b), c, "#{a} >> #{b}")
        end
      }
    }
  end

  def test_succ
    VS.each {|a|
      b = a.succ
      assert_equal(a+1, b, "(#{a}).succ")
      assert_equal(a, b.pred, "(#{a}).succ.pred")
      assert_equal(a, b-1, "(#{a}).succ - 1")
    }
  end

  def test_pred
    VS.each {|a|
      b = a.pred
      assert_equal(a-1, b, "(#{a}).pred")
      assert_equal(a, b.succ, "(#{a}).pred.succ")
      assert_equal(a, b + 1, "(#{a}).pred + 1")
    }
  end

  def test_unary_plus
    VS.each {|a|
      b = +a
      assert_equal(a, b, "+(#{a})")
    }
  end

  def test_unary_minus
    VS.each {|a|
      b = -a
      assert_equal(0-a, b, "-(#{a})")
      assert_equal(~a+1, b, "-(#{a})")
      assert_equal(0, a+b, "#{a}+(-(#{a}))")
    }
  end

  def test_cmp
    VS.each_with_index {|a, i|
      VS.each_with_index {|b, j|
        assert_equal(i <=> j, a <=> b, "#{a} <=> #{b}")
        assert_equal(i < j, a < b, "#{a} < #{b}")
        assert_equal(i <= j, a <= b, "#{a} <= #{b}")
        assert_equal(i > j, a > b, "#{a} > #{b}")
        assert_equal(i >= j, a >= b, "#{a} >= #{b}")
      }
    }
  end

  def test_eq
    VS.each_with_index {|a, i|
      VS.each_with_index {|b, j|
        c = a == b
        assert_equal(b == a, c, "#{a} == #{b}")
        assert_equal(i == j, c, "#{a} == #{b}")
      }
    }
  end

  def test_abs
    VS.each {|a|
      b = a.abs
      if a < 0
        assert_equal(-a, b, "(#{a}).abs")
      else
        assert_equal(a, b, "(#{a}).abs")
      end
    }
  end

  def test_ceil
    VS.each {|a|
      assert_equal(a, a.ceil, "(#{a}).ceil")
    }
  end

  def test_floor
    VS.each {|a|
      assert_equal(a, a.floor, "(#{a}).floor")
    }
  end

  def test_round
    VS.each {|a|
      assert_equal(a, a.round, "(#{a}).round")
    }
  end

  def test_truncate
    VS.each {|a|
      assert_equal(a, a.truncate, "(#{a}).truncate")
    }
  end

  def test_remainder
    VS.each {|a|
      VS.each {|b|
        next if b == 0
        r = a.remainder(b)
        if a < 0
          assert_operator(-b.abs, :<, r, "#{a}.remainder(#{b})")
          assert_operator(0, :>=, r, "#{a}.remainder(#{b})")
        elsif 0 < a
          assert_operator(0, :<=, r, "#{a}.remainder(#{b})")
          assert_operator(b.abs, :>, r, "#{a}.remainder(#{b})")
        else
          assert_equal(0, r, "#{a}.remainder(#{b})")
        end
      }
    }
  end

  def test_zero_nonzero
    VS.each {|a|
      z = a.zero?
      n = a.nonzero?
      if a == 0
        assert_equal(true, z, "(#{a}).zero?")
        assert_equal(nil, n, "(#{a}).nonzero?")
      else
        assert_equal(false, z, "(#{a}).zero?")
        assert_equal(a, n, "(#{a}).nonzero?")
      end
      assert(z ^ n, "(#{a}).zero? ^ (#{a}).nonzero?")
    }
  end

  def test_even_odd
    VS.each {|a|
      e = a.even?
      o = a.odd?
      assert_equal((a % 2) == 0, e, "(#{a}).even?")
      assert_equal((a % 2) == 1, o, "(#{a}).odd")
      assert_equal((a & 1) == 0, e, "(#{a}).even?")
      assert_equal((a & 1) == 1, o, "(#{a}).odd")
      assert(e ^ o, "(#{a}).even? ^ (#{a}).odd?")
    }
  end

  def test_Integer
    assert_raise(ArgumentError) {Integer("0x-1")}
    assert_raise(ArgumentError) {Integer("-0x-1")}
    assert_raise(ArgumentError) {Integer("0x     123")}
    assert_raise(ArgumentError) {Integer("0x      123")}
    assert_raise(ArgumentError) {Integer("0x0x5")}
    assert_raise(ArgumentError) {Integer("0x0x000000005")}
    assert_nothing_raised(ArgumentError) {
      assert_equal(1540841, "0x0x5".to_i(36))
    }
  end
end
