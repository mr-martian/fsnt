FILE* input = NULL;
UFILE* output = NULL;
{
  #include "tools/cli/get_io.cc"
  input = input_file;
  output = u_fadopt(output_file, NULL, NULL);
}
