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

#include <platform.h>
#include <csv_writer.h>

#include <alloc.h>

struct CsvWriter_
{
    Writer *w;
    bool beginning_of_line;
    bool terminate_last_line;
};

/*****************************************************************************/

static void WriteCsvEscapedString(Writer *w, const char *str);
static void CsvWriterFieldVF(CsvWriter * csvw, const char *fmt, va_list ap) FUNC_ATTR_PRINTF(2, 0);

/*****************************************************************************/

CsvWriter *CsvWriterOpenSpecifyTerminate(Writer *w, bool terminate_last_line)
{
    CsvWriter *csvw = xmalloc(sizeof(CsvWriter));

    csvw->w = w;
    csvw->beginning_of_line = true;
    csvw->terminate_last_line = terminate_last_line;
    return csvw;
}

CsvWriter *CsvWriterOpen(Writer *w)
{
    return CsvWriterOpenSpecifyTerminate(w, true);
}

/*****************************************************************************/

void CsvWriterField(CsvWriter * csvw, const char *str)
{
    assert(csvw != NULL);

    if (csvw->beginning_of_line)
    {
        csvw->beginning_of_line = false;
    }
    else
    {
        WriterWriteChar(csvw->w, ',');
    }

    if (strpbrk(str, "\",\r\n"))
    {
        WriteCsvEscapedString(csvw->w, str);
    }
    else
    {
        WriterWrite(csvw->w, str);
    }
}

/*****************************************************************************/

void CsvWriterFieldF(CsvWriter * csvw, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    CsvWriterFieldVF(csvw, fmt, ap);
    va_end(ap);
}

/*****************************************************************************/

void CsvWriterNewRecord(CsvWriter * csvw)
{
    assert(csvw != NULL);

    WriterWrite(csvw->w, "\r\n");
    csvw->beginning_of_line = true;
}

/*****************************************************************************/

void CsvWriterClose(CsvWriter * csvw)
{
    assert(csvw != NULL);

    if (!csvw->beginning_of_line && csvw->terminate_last_line)
    {
        WriterWrite(csvw->w, "\r\n");
    }
    free(csvw);
}

/*****************************************************************************/

static void CsvWriterFieldVF(CsvWriter * csvw, const char *fmt, va_list ap)
{
/*
 * We are unable to avoid allocating memory here, as we need full string
 * contents before deciding whether there is a " in string, and hence whether
 * the field needs to be escaped in CSV
 */
    char *str;

    xvasprintf(&str, fmt, ap);
    CsvWriterField(csvw, str);
    free(str);
}

/*****************************************************************************/

static void WriteCsvEscapedString(Writer *w, const char *s)
{
    WriterWriteChar(w, '"');
    while (*s)
    {
        if (*s == '"')
        {
            WriterWriteChar(w, '"');
        }
        WriterWriteChar(w, *s);
        s++;
    }
    WriterWriteChar(w, '"');
}

Writer *CsvWriterGetWriter(CsvWriter *csvw)
{
    assert(csvw != NULL);
    return csvw->w;
}
