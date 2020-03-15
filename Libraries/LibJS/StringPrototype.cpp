/*
 * Copyright (c) 2020, Andreas Kling <kling@serenityos.org>
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

#include <AK/Function.h>
#include <LibJS/Heap.h>
#include <LibJS/Interpreter.h>
#include <LibJS/PrimitiveString.h>
#include <LibJS/StringObject.h>
#include <LibJS/StringPrototype.h>
#include <LibJS/Value.h>

namespace JS {

StringPrototype::StringPrototype()
{
    put_native_function("charAt", [](Interpreter& interpreter, Vector<Value> arguments) -> Value {
        i32 index = 0;
        if (!arguments.is_empty())
            index = arguments[0].to_i32();
        Value this_value = interpreter.this_value();
        ASSERT(this_value.is_object());
        ASSERT(this_value.as_object()->is_string_object());
        auto underlying_string = static_cast<const StringObject*>(this_value.as_object())->primitive_string()->string();
        if (index < 0 || index >= static_cast<i32>(underlying_string.length()))
            return js_string(interpreter.heap(), String::empty());
        return js_string(interpreter.heap(), underlying_string.substring(index, 1));
    });
}

StringPrototype::~StringPrototype()
{
}

}