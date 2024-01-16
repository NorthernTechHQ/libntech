/*
  Copyright 2024 Northern.tech AS

  This file is part of CFEngine 3 - written and maintained by Northern.tech AS.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  To the extent this program is licensed as part of the Enterprise
  versions of CFEngine, the applicable Commercial Open Source License
  (COSL) may apply to this file if you as a licensee so wish it. See
  included file COSL.txt.
*/
#ifndef CFENGINE_PRINTSIZE_H
#define CFENGINE_PRINTSIZE_H

/** Size of buffer needed to sprintf() an integral value.
 *
 * The constant 53/22 is a (very slightly) high approximation to
 * log(256)/log(10), the number of decimal digits needed per byte of
 * the value.  The offset of 3 accounts for: rounding details, the
 * '\0' you need at the end of the buffer and the sign.  The last is a
 * wasted byte when using "%u", but it's simpler this way !
 *
 * This macro should be preferred over using a char [64] buffer (or
 * 128 or 32 or whatever guess you've made at "big enough") and
 * trusting that your integer shall fit - for now it saves wasting
 * stack space; one day it may even (when we have bigger integral
 * types) save us a buffer over-run.  Limitation: if CHAR_BIT ever
 * isn't 8, we should change the / 22 to * CHAR_BIT / 176.
 *
 * This deals with the simple case of a "%d", possibly with type
 * modifiers (jhzl, etc.) or sign (%+d).  If you're using fancier
 * modifiers (e.g. field-width), you need to think about what effect
 * they have on the print width (max of field-width and this macro);
 * if there is other text in your format, you need to allow space for
 * it, too.
 *
 * If you are casting the value to be formatted (e.g. because there's
 * no modifier for its type), applying this macro to the value shall
 * get the maximum size that a %d-based format can produce for it (its
 * value can't be any bigger, even if it's cast to a huge type); but,
 * if you're using a %u-based format and the value is signed, you
 * should call this macro on the type to which you're casting it
 * (because, if it's negative, it'll wrap to a huge value of the cast
 * type).
 *
 * @param #what The integral expression or its type.  Is not evaluated
 * at run-time, only passed to sizeof() at compile-time.
 *
 * @return The size for a buffer big enough to hold sprintf()'s
 * representation of an integer of this type.  This is a compile-time
 * constant, so can be used in static array declarations or struct
 * member array declarations.
 */
#define PRINTSIZE(what) (3 + 53 * sizeof(what) / 22)

#endif /* CFENGINE_PRINTSIZE_H */
