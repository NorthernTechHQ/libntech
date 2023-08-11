/*
  Copyright 2023 Northern.tech AS

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

#include <statistics.h>

#include <platform.h>

/**********************************************************************/

double GAverage(double anew, double aold, double p)
/* return convex mixture - p is the trust/confidence in the new value */
{
    return (p * anew + (1.0 - p) * aold);
}

/*
 * expected(Q) = p*Q_new + (1-p)*expected(Q)
 * variance(Q) = p*(Q_new - expected(Q))^2 + (1-p)*variance(Q)
 */

/**********************************************************************/

QPoint QAverage(QPoint old, double new_q, double p)
{
    QPoint new = {
        .q = new_q,
    };

    double devsquare = (new.q - old.expect) * (new.q - old.expect);

    new.dq = new.q - old.q;
    new.expect = GAverage(new.q, old.expect, p);
    new.var = GAverage(devsquare, old.var, p);

    return new;
}

/**********************************************************************/

QPoint QDefinite(double q)
{
    return (QPoint) { .q = q, .dq = 0.0, .expect = q, .var = 0.0 };
}
