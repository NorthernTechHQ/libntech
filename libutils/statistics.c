/*
  Copyright 2022 Northern.tech AS

  This file is part of CFEngine 3 - written and maintained by Northern.tech AS.

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the
  Free Software Foundation; version 3.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA

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
    QPoint _new = {
        .q = new_q,
    };

    double devsquare = (_new.q - old.expect) * (_new.q - old.expect);

    _new.dq = _new.q - old.q;
    _new.expect = GAverage(_new.q, old.expect, p);
    _new.var = GAverage(devsquare, old.var, p);

    return _new;
}

/**********************************************************************/

QPoint QDefinite(double q)
{
    return (QPoint) { .q = q, .expect = q, .var = 0.0, .dq = 0.0 };
}
