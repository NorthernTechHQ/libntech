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

#include <xml_writer.h>

#include <misc_lib.h>

static void WriteEscaped(Writer *writer, const char *source);

/*****************************************************************************/

void XmlComment(Writer *writer, const char *comment)
{
    if (writer == NULL)
    {
        ProgrammingError("NULL writer passed to XmlWriter");
    }

    WriterWrite(writer, "<!-- ");
    WriteEscaped(writer, comment);
    WriterWrite(writer, " -->\n");
}

/*****************************************************************************/

static void XmlEmitStartTag(Writer *writer, const char *tag_name, int attr_cnt, va_list args)
{
    WriterWriteF(writer, "<%s", tag_name);

    if (attr_cnt > 0)
    {
        WriterWrite(writer, " ");
    }

    for (int i = 0; i < attr_cnt; ++i)
    {
        XmlAttribute attr = va_arg(args, XmlAttribute);

        WriterWriteF(writer, "%s=\"%s\" ", attr.name, attr.value);
    }

    WriterWrite(writer, ">");
}

/*****************************************************************************/

void XmlStartTag(Writer *writer, const char *tag_name, int attr_cnt, ...)
{
    va_list args;

    if ((writer == NULL) || (tag_name == NULL) || (attr_cnt < 0))
    {
        ProgrammingError("writer, tag_name or attr_cnt in XmlStartTag are wrong");
    }

    va_start(args, attr_cnt);
    XmlEmitStartTag(writer, tag_name, attr_cnt, args);
    va_end(args);

    WriterWrite(writer, "\n");
}

/*****************************************************************************/

void XmlEndTag(Writer *writer, const char *tag_name)
{
    if ((writer == NULL) || (tag_name == NULL))
    {
        ProgrammingError("writer or tag_name are missing");
    }

    WriterWriteF(writer, "</%s>\n", tag_name);
}

/*****************************************************************************/

void XmlTag(Writer *writer, const char *tag_name, const char *value, int attr_cnt, ...)
{
    va_list args;

    if ((writer == NULL) || (tag_name == NULL) || (attr_cnt < 0))
    {
        return;
    }

    va_start(args, attr_cnt);
    XmlEmitStartTag(writer, tag_name, attr_cnt, args);
    va_end(args);

    if (value != NULL)
    {
        WriteEscaped(writer, value);
    }

    XmlEndTag(writer, tag_name);
}

/*****************************************************************************/

void XmlContent(Writer *writer, const char *value)
{
    if (writer == NULL)
    {
        ProgrammingError("writer is NULL");
    }

    WriteEscaped(writer, value);
}

/*****************************************************************************/

static void WriteEscaped(Writer *w, const char *source)
{
    for (const char *s = source; *s; s++)
    {
        switch (*s)
        {
        case '&':
            WriterWrite(w, "&amp;");
            break;
        case '>':
            WriterWrite(w, "&gt;");
            break;
        case '"':
            WriterWrite(w, "&quot;");
            break;
        case '\'':
            WriterWrite(w, "&apos;");
            break;
        case '<':
            WriterWrite(w, "&lt;");
            break;
        default:
            WriterWriteChar(w, *s);
        }
    }
}
