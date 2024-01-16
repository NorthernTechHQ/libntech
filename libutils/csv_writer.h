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

#ifndef CFENGINE_CSV_WRITER_H
#define CFENGINE_CSV_WRITER_H

/*
 * This writer implements CSV as in RFC 4180
 */

#include <writer.h>

typedef struct CsvWriter_ CsvWriter;

CsvWriter *CsvWriterOpenSpecifyTerminate(Writer *w, bool terminate_last_line);
CsvWriter *CsvWriterOpen(Writer *w);

void CsvWriterField(CsvWriter *csvw, const char *str);
void CsvWriterFieldF(CsvWriter *csvw, const char *fmt, ...) FUNC_ATTR_PRINTF(2, 3);

void CsvWriterNewRecord(CsvWriter *csvw);

/* Does not close underlying Writer, but flushes all pending data */
void CsvWriterClose(CsvWriter *csvw);

/**
 * @return The instance of the underlying writer
 */
Writer *CsvWriterGetWriter(CsvWriter *csvw);

#endif
