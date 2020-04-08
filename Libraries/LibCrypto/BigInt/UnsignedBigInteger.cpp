/*
 * Copyright (c) 2020, Itamar S. <itamar8910@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "UnsignedBigInteger.h"

namespace Crypto {

/**
 * Complexity: O(N) where N is the number of words in the larger number
 */
UnsignedBigInteger UnsignedBigInteger::add(const UnsignedBigInteger& other) const
{
    const UnsignedBigInteger* const longer = (length() > other.length()) ? this : &other;
    const UnsignedBigInteger* const shorter = (longer == &other) ? this : &other;
    UnsignedBigInteger result;

    u8 carry = 0;
    for (size_t i = 0; i < shorter->length(); ++i) {
        u32 word_addition_result = shorter->m_words[i] + longer->m_words[i];
        u8 carry_out = 0;
        // if there was a carry, the result will be smaller than any of the operands
        if (word_addition_result + carry < shorter->m_words[i]) {
            carry_out = 1;
        }
        if (carry) {
            word_addition_result++;
        }
        carry = carry_out;
        result.m_words.append(word_addition_result);
    }

    for (size_t i = shorter->length(); i < longer->length(); ++i) {
        u32 word_addition_result = longer->m_words[i] + carry;

        carry = 0;
        if (word_addition_result < longer->m_words[i]) {
            carry = 1;
        }
        result.m_words.append(word_addition_result);
    }
    if (carry) {
        result.m_words.append(carry);
    }
    return result;
}

/**
 * Complexity: O(N) where N is the number of words in the larger number
 */
UnsignedBigInteger UnsignedBigInteger::sub(const UnsignedBigInteger& other) const
{
    UnsignedBigInteger result;

    if (*this < other) {
        dbg() << "WARNING: bigint subtraction creates a negative number!";
        return UnsignedBigInteger::create_invalid();
    }

    u8 borrow = 0;
    for (size_t i = 0; i < other.length(); ++i) {
        ASSERT(!(borrow == 1 && m_words[i] == 0));

        if (m_words[i] - borrow < other.m_words[i]) {
            u64 after_borrow = static_cast<u64>(m_words[i] - borrow) + (UINT32_MAX + 1);
            result.m_words.append(static_cast<u32>(after_borrow - static_cast<u64>(other.m_words[i])));
            borrow = 1;
        } else {
            result.m_words.append(m_words[i] - borrow - other.m_words[i]);
            borrow = 0;
        }
    }

    for (size_t i = other.length(); i < length(); ++i) {
        ASSERT(!(borrow == 1 && m_words[i] == 0));
        result.m_words.append(m_words[i] - borrow);
        borrow = 0;
    }

    return result;
}

/**
 * Complexity: O(N^2) where N is the number of words in the larger number
 * Multiplcation method:
 * An integer is equal to the sum of the powers of two
 * according to the indexes of its 'on' bits.
 * So to multiple x*y, we go over each '1' bit in x (say the i'th bit), 
 * and add y<<i to the result.
 */
UnsignedBigInteger UnsignedBigInteger::multiply(const UnsignedBigInteger& other) const
{
    UnsignedBigInteger result;
    // iterate all bits
    for (size_t word_index = 0; word_index < length(); ++word_index) {
        for (size_t bit_index = 0; bit_index < UnsignedBigInteger::BITS_IN_WORD; ++bit_index) {
            // If the bit is off - skip over it
            if (!(m_words[word_index] & (1 << bit_index)))
                continue;

            const size_t shift_amount = word_index * UnsignedBigInteger::BITS_IN_WORD + bit_index;
            auto shift_result = other.shift_left(shift_amount);
            result = result.add(shift_result);
        }
    }
    return result;
}

UnsignedBigInteger UnsignedBigInteger::shift_left(size_t num_bits) const
{
    // We can only do shift operations on individual words
    // where the shift amount is <= size of word (32).
    // But we do know how to shift by a multiple of word size (e.g 64=32*2)
    // So we first shift the result by how many whole words fit in 'num_bits'
    UnsignedBigInteger temp_result = shift_left_by_n_words(num_bits / UnsignedBigInteger::BITS_IN_WORD);

    // And now we shift by the leftover amount of bits
    num_bits %= UnsignedBigInteger::BITS_IN_WORD;

    UnsignedBigInteger result(temp_result);

    for (size_t i = 0; i < temp_result.length(); ++i) {
        u32 current_word_of_temp_result = temp_result.shift_left_get_one_word(num_bits, i);
        result.m_words[i] = current_word_of_temp_result;
    }

    // Shifting the last word can produce a carry
    u32 carry_word = temp_result.shift_left_get_one_word(num_bits, temp_result.length());
    if (carry_word != 0) {
        result = result.add(UnsignedBigInteger(carry_word).shift_left_by_n_words(temp_result.length()));
    }
    return result;
}

UnsignedBigInteger UnsignedBigInteger::shift_left_by_n_words(const size_t number_of_words) const
{
    // shifting left by N words means just inserting N zeroes to the beginning of the words vector
    UnsignedBigInteger result;
    for (size_t i = 0; i < number_of_words; ++i) {
        result.m_words.append(0);
    }
    for (size_t i = 0; i < length(); ++i) {
        result.m_words.append(m_words[i]);
    }
    return result;
}

/**
 * Returns the word at a requested index in the result of a shift operation
 */
u32 UnsignedBigInteger::shift_left_get_one_word(const size_t num_bits, const size_t result_word_index) const
{
    // "<= length()" (rather than length() - 1) is intentional,
    // The result inedx of length() is used when calculating the carry word
    ASSERT(result_word_index <= length());
    ASSERT(num_bits <= UnsignedBigInteger::BITS_IN_WORD);
    u32 result = 0;

    // we need to check for "num_bits != 0" since shifting right by 32 is apparently undefined behaviour!
    if (result_word_index > 0 && num_bits != 0) {
        result += m_words[result_word_index - 1] >> (UnsignedBigInteger::BITS_IN_WORD - num_bits);
    }
    if (result_word_index < length() && num_bits < 32) {
        result += m_words[result_word_index] << num_bits;
    }
    return result;
}

bool UnsignedBigInteger::operator==(const UnsignedBigInteger& other) const
{
    if (trimmed_length() != other.trimmed_length()) {
        return false;
    }
    if (is_invalid() != other.is_invalid()) {
        return false;
    }

    for (size_t i = 0; i < trimmed_length(); ++i) {
        if (m_words[i] != other.words()[i])
            return false;
    }
    return true;
}

bool UnsignedBigInteger::operator<(const UnsignedBigInteger& other) const
{
    if (trimmed_length() < other.trimmed_length()) {
        return true;
    }
    if (trimmed_length() > other.trimmed_length()) {
        return false;
    }

    size_t length = trimmed_length();
    if (length == 0) {
        return false;
    }

    return m_words[length - 1] < other.m_words[length - 1];
}

size_t UnsignedBigInteger::trimmed_length() const
{
    size_t num_leading_zeroes = 0;
    for (int i = length() - 1; i >= 0; --i, ++num_leading_zeroes) {
        if (m_words[i] != 0)
            break;
    }
    return length() - num_leading_zeroes;
}

UnsignedBigInteger UnsignedBigInteger::create_invalid()
{
    UnsignedBigInteger invalid(0);
    invalid.invalidate();
    return invalid;
}

}