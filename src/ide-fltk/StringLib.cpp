// -*- c-file-style: "java" -*-
// $Id: StringLib.cpp,v 1.3 2005-03-09 23:01:00 zeeb90au Exp $
// This file is part of EBjLib
//
// Copyright(C) 2001-2004 Chris Warren-Smith. Gawler, South Australia
// cwarrens@twpo.com.au
//
// This program is distributed under the terms of the GPL v2.0 or later
// Download the GNU Public License (GPL) from www.gnu.org
//

#include "StringLib.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace strlib;

//--Object----------------------------------------------------------------------

Object::Object() {}
Object::~Object() {}

//--String----------------------------------------------------------------------

String::String(const char* s)  {
    owner=false;
    buffer=(char*)s;
}

String::String(const String& s)  {
    init();
    append(s.buffer);
}

String::String() {
    init();
}

String::~String() {
    empty();
}

const String& String::operator=(const String& s) {
    if (this != &s) {
        empty();
        append(s.buffer);
    }
    return *this;
}

const String& String::operator=(const char* s) {
    empty();
    append(s);
    return *this;
}

const String& String::operator=(const char c) {
    empty();
    append(&c, 1);
    return *this;
}

const void String::operator+=(const String& s) {
    append(s.buffer);
}

const void String::operator+=(const char* s) {
    append(s);
}

const String String::operator+(const String& s) {
    String rs;
    rs.append(buffer);
    rs.append(s.buffer);
    return rs;
}

const String String::operator+(int i) {
    String rs;
    rs.append(buffer);
    rs.append(i);
    return rs;
}

const String& String::operator=(int i) {
    empty();
    append(i);
    return *this;
}

void String::append(const String& s) {
    append(s.buffer);
}

void String::append(int i) {
    char t[20];
    sprintf(t, "%i", i);
    append(t);
}

void String::append(double d) {
    char t[20];
    sprintf(t, "%g", d);
    append(t);
}

void String::append(int i, int padding) {
    char buf[20];
    char fmt[20];
    fmt[0]='%';
    fmt[1]='0';
    padding = min(20,padding);
    sprintf(fmt+2, "%dd", padding);
    sprintf(buf, fmt, i);
    append(buf);
}

void String::append(const char *s) {
    if (s != null && owner) {
        int len = length();
        buffer = (char*)realloc(buffer, len + strlen(s) + 1);
        strcpy(buffer+len, s);
    }
}

void String::append(const char *s, int numCopy) {
    if (!owner || s == null || numCopy < 1) {
        return;
    }
    int len = strlen(s);
    if (numCopy > len) {
        numCopy = len;
    }
    
    len = length();
    buffer = (char*)realloc(buffer, len + numCopy + 1);
    strncpy(buffer+len, s, numCopy);
    buffer[len + numCopy] = '\0';
}

void String::append(FILE *fp, long filelen) {
    int len = length();
    buffer = (char*)realloc(buffer, len+filelen + 1);
    fread((void *)(len+buffer), 1, filelen, fp);
    buffer[len+filelen] = 0;
}

const char* String::toString() const {
    return buffer;
}

int String::length() const {
    return (buffer == 0 ? 0 : strlen(buffer));
}

String String::substring(int beginIndex) const {
    String out;
    if (beginIndex < length()) {
        out.append(buffer+beginIndex);
    }
    return out;
}

String String::substring(int beginIndex, int endIndex) const {
    String out;
    int len = length();
    if (endIndex > len) {
        endIndex = len;
    }
    if (beginIndex < length()) {
        out.append(buffer+beginIndex, endIndex-beginIndex);
    }
    return out;
}

void String::replaceAll(char a, char b) {
    int len = length();
    for (int i=0; i<len; i++) {
        if (buffer[i]==a) {
            buffer[i]=b;
        }
    }
}

String String::replaceAll(const char* srch, const char* repl) {
    String out;
    int begin = 0;
    int numCopy = 0;
    int numMatch = 0;
    int len = length();
    int srchLen = strlen(srch);

    for (int i=0; i<len; i++) {
        numMatch = (buffer[i] == srch[numMatch]) ? numMatch+1 : 0;
        if (numMatch == srchLen) {
            numCopy = 1+i-begin-srchLen;
            if (numCopy > 0) {
                out.append(buffer+begin, numCopy);
            }
            out.append(repl);
            numMatch = 0;
            begin = i+1;
        }
    }

    if (begin < len) {
        out.append(buffer+begin);
    }
    return out;
}

void String::toUpperCase() {
    int len = length();
    for (int i=0; i<len; i++) {
        buffer[i] = toupper(buffer[i]);
    }
}

void String::toLowerCase() {
    int len = length();
    for (int i=0; i<len; i++) {
        buffer[i] = tolower(buffer[i]);
    }
}

int String::toInteger() const {
    return (buffer == 0 ? 0 : atoi(buffer));
}

double String::toNumber() const {
    return (buffer == 0 ? 0 : atof(buffer));
}

bool String::equals(const String &s) const {
    return (buffer == 0 ? s.buffer == 0 : strcmp(buffer, s.buffer) == 0);
}

bool String::equals(const char* s) const {
    return (buffer == 0 ? s == 0 : strcmp(buffer, s) == 0);
}

bool String::equalsIgnoreCase(const char* s) const {
    return (stricmp(buffer, s) == 0);
}

bool String::startsWith(const String &s) const {
    return (strnicmp(buffer, s.buffer, s.length()) == 0);
}

bool String::startsWithIgnoreCase(const String &s) const {
    return (strnicmp(buffer, s.buffer, s.length()) == 0);
}

int String::indexOf(const String &s, int fromIndex) const {
    int len = length();
    if (fromIndex>=len) {
        return -1;
    }
    if (s.length()==1) {
        char* c = strchr(buffer+fromIndex, s.buffer[0]);
        return (c==NULL ? -1 : (c-buffer));
    } else {
        char* c = strstr(buffer+fromIndex, s.buffer);
        return (c==NULL ? -1 : (c-buffer));
    }
}

int String::indexOf(char chr, int fromIndex) const {
    int len = length();
    if (fromIndex>=len) {
        return -1;
    }
    char* c = strchr(buffer+fromIndex, chr);
    return (c==NULL ? -1 : (c-buffer));
}

char String::charAt(int i) const {
    if (i<length()) {
        return buffer[i];
    }
    return -1;
}

void String::empty() {
    if (buffer != null && owner) {
        free(buffer);
    }
    buffer=0;
}

String String::trim() const {
    String s(buffer);
    int len = s.length();
    bool whiteSp = false;
    for (int i=0; i<len; i++) {
        if (s.buffer[i] == '\r' ||
            s.buffer[i] == '\n' ||
            s.buffer[i] == '\t') {
            s.buffer[i] = ' ';
            whiteSp = true;
        } else if (i<len-1 &&
            s.buffer[i] == ' ' && 
            s.buffer[i+1] == ' ') {
            whiteSp = true;
        }
    }
    String b(s.buffer);
    if (whiteSp) {
        int iAppend=0;
        for (int i=0; i<len; i++) {
            if (i<len-1 &&
                s.buffer[i] == ' ' && 
                s.buffer[i+1] == ' ') {
                continue;
            }
            b.buffer[iAppend++] = s.buffer[i];
        }
        b.buffer[iAppend] = '\0';
    }
    return b;
}

String String::lvalue() {
    int endIndex = indexOf('=', 0);
    if (endIndex == -1) {
        return null;
    }
    return substring(0, endIndex);
}

String String::rvalue() {
    int endIndex = indexOf('=', 0);
    if (endIndex == -1) {
        return null;
    }
    return substring(endIndex+1, length());
}

//--List------------------------------------------------------------------------

List::List(int growSize) {
    this->growSize = growSize;
    init();
}

List::~List() {
    for (int i=0; i<count; i++) {
        delete head[i];
    }
    free(head);
    head = 0;
    count = 0;
}

void List::init() {
    count= 0;
    size = growSize;
    head = (Object**)malloc(sizeof(Object*)*size);
}

void List::removeAll() {
    for (int i=0; i<count; i++) {
        delete head[i];
    }
    emptyList();
}

void List::emptyList() {
    free(head);
    init();
}

Object* List::operator[](const int index) const {
    return index < count ? head[index] : 0;
}

Object* List::get(const int index) const {
    return index < count ? head[index] : 0;
}

void List::append(Object* object) {
    if (++count > size) {
        size += growSize;
        head = (Object**)realloc(head, sizeof(Object*)*size);
    }
    head[count-1] = object;
}

void List::iterateInit(int ibegin /*=0*/) {
    iterator = ibegin;
}

bool List::hasNext() const {
    return (iterator < count);
}

Object* List::next() {
    return head[iterator++];
}

const char** List::toArray() {
    if (length()) {
        int i=0;
        const char** array = new const char*[length()];
        iterateInit();
        while (hasNext()) {
            array[i++] = ((String*)next())->toString();
        }
        return array;
    }
    return 0;
}

//--Stack-----------------------------------------------------------------------

Stack::Stack() : List() {}

Stack::Stack(int growSize) : List(growSize) {}

Object* Stack::peek() {
    if (count == 0) {
        return 0;
    }
    return head[count-1];
}

Object* Stack::pop() {
    if (count == 0) {
        return 0;
    }
    return head[--count];
}

void Stack::push(Object* o) {
    append(o);
}

//--Properties------------------------------------------------------------------

Properties::Properties(int growSize) : 
    list(growSize) {
}

Properties::Properties() {}

Properties::~Properties() {}

void Properties::load(const char *s) {
    if (s == 0 || s[0] == 0) {
        return;
    }
    load(s, strlen(s));
}

void Properties::load(const char *s, int slen) {
    if (s == 0 || s[0] == 0 || slen == 0) {
        return;
    }

    String attr;
    String value;

    int i=0;
    while (i<slen) {
        attr.empty();
        value.empty();

        // remove w/s before attribute
        while (i<slen && isWhite(s[i])) {
            i++;
        }
        if (i == slen) {
            break;
        }
        int iBegin = i;

        // find end of attribute
        while (i<slen && s[i] != '=' && !isWhite(s[i])) {
            i++;
        }
        if (i == slen) {
            break;
        }

        attr.append(s+iBegin, i-iBegin);

        // scan for equals 
        while (i<slen && isWhite(s[i])) {
            i++;
        }
        if (i == slen) {
            break;
        }

        if (s[i] != '=') {
            break;
        }
        i++; // skip equals

        // scan value
        while (i<slen && isWhite(s[i])) {
            i++;
        }
        if (i == slen) {
            break;
        }

        if (s[i] == '\"' || s[i] == '\'') {
            // scan quoted value
            char quote = s[i];
            iBegin = ++i;
            while (i<slen && s[i] != quote) {
                i++;
            }
        } else {
            // non quoted value
            iBegin = i;
            while (i<slen && !isWhite(s[i])) {
                i++;
            }
        }

        value.append(s+iBegin, i-iBegin);
        // append (put) to list
        list.append(new String(attr));
        list.append(new String(value));
        i++;
    }
}

String* Properties::get(const char* key) {
    list.iterateInit();
    while (list.hasNext()) {
        String *nextKey = (String*)list.next();
        if (nextKey == null || list.hasNext()==false) {
            return null;
        }
        String *nextValue = (String*)list.next();
        if (nextValue == null) {
            return null;
        }
        if (nextKey->equals(key)) {
            return nextValue;
        }
    }
    return null;
}

void Properties::get(const char* key, List* arrayValues) {
    list.iterateInit();
    while (list.hasNext()) {
        String *nextKey = (String*)list.next();
        if (nextKey == null || list.hasNext()==false) {
            break;
        }
        String *nextValue = (String*)list.next();
        if (nextValue == null) {
            break;
        }
        if (nextKey->equals(key)) {
            arrayValues->append(new String(*nextValue));
        }
    }
}

void Properties::put(String& key, String& value) {
    String* prev = get(key.toString());
    if (prev) {
        prev->empty();
        prev->append(value);
    } else {
        list.append(new String(key));
        list.append(new String(value));
    }
}

void Properties::put(const char* key, const char* value) {
    String* prev = get(key);
    if (prev) {
        prev->empty();
        prev->append(value);
    } else {
        String* k = new String();
        String* v = new String();
        k->append(key);
        v->append(value);
        list.append(k);
        list.append(v);
    }
}

String Properties::toString() {
    String s;
    list.iterateInit();
    while (list.hasNext()) {
        String *nextKey = (String*)list.next();
        if (nextKey == null || nextKey->length() == 0 ||
            list.hasNext()==false) {
            break;
        }
        String *nextValue = (String*)list.next();
        if (nextValue != null && nextValue->length() > 0) {
            s.append(nextKey->toString());
            s.append("='");
            s.append(nextValue->toString());
            s.append("'\n");
        }
    }
    return s;
}

void Properties::operator=(Properties& p) {
    list.removeAll();
    p.list.iterateInit();
    while (p.list.hasNext()) {
        list.append(p.list.next());
    }
    p.list.emptyList();
}

//--EndOfFile-------------------------------------------------------------------

