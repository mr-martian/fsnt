UFILE* input = NULL;
FILE* output = NULL;
{
  #include "tools/cli/get_io.cc"
  input = u_fadopt(input_file, NULL, NULL);
  output = output_file;
}
