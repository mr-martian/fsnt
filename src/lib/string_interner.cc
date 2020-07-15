#include "string_interner.h"
#include "utils/compression.h"

StringInterner::StringInterner()
{
  internName("");
}

StringInterner::~StringInterner()
{
}

const UnicodeString&
StringInterner::name(string_ref r) const
{
  return id_to_name[(unsigned int)r];
}

string_ref
StringInterner::internName(const UnicodeString& name)
{
  if(name_to_id.find(name) == name_to_id.end())
  {
    name_to_id[name] = string_ref(id_to_name.size());
    id_to_name.push_back(name);
  }
  return name_to_id[name];
}

void
StringInterner::read(FILE* in)
{
  id_to_name.clear();
  name_to_id.clear();
  internName("");
  for(unsigned int i = 1, lim = Compression::multibyte_read(in); i < lim; i++) {
    id_to_name.push_back(Compression::string_read(in));
    name_to_id[id_to_name.back()] = string_ref(i);
  }
}

void
StringInterner::write(FILE* out)
{
  Compression::multibyte_write(id_to_name.size(), out);
  for(unsigned int i = 1; i < id_to_name.size(); i++) {
    Compression::string_write(id_to_name[i], out);
  }
}

void
StringInterner::write_symbol(UFILE* out, string_ref sym, bool escape)
{
  UnicodeString& s = id_to_name[(unsigned int)sym];
  if(escape) {
    if(s == " ") {
      u_fprintf(out, "@_SPACE_@");
      return;
    } else if(s == "\t") {
      u_fprintf(out, "@_TAB_@");
      return;
    } else if(s == "") {
      u_fprintf(out, "@0@");
      return;
    }
  }
  u_fprintf(out, "%S", s.getTerminatedBuffer());
}

std::map<string_ref, string_ref>
StringInterner::merge(StringInterner& other)
{
  std::map<string_ref, string_ref> ret;
  for(size_t i = 1; i < other.id_to_name.size(); i++) {
    ret[string_ref(i)] = internName(other.id_to_name[i]);
  }
  return ret;
}
