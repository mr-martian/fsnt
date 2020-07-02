FILE* input_file = stdin;
FILE* output_file = stdout;
{
  string infile;
  string outfile;
  switch(argc - optind)
  {
    case 0:
      break;

    case 1:
      infile = argv[argc-1];
      break;

    case 2:
      infile = argv[argc-2];
      outfile = argv[argc-1];
      break;

    default:
      endProgram(argv[0]);
      break;
  }

  if(infile != "" && infile != "-")
  {
    input_file = fopen(infile.c_str(), "rb");
    if(!input_file)
    {
      std::cerr << "Error: Cannot open file '" << infile << "' for reading." << std::endl;
      exit(EXIT_FAILURE);
    }
  }

  if(outfile != "" && outfile != "-")
  {
    output_file = fopen(outfile.c_str(), "wb");
    if(!output_file)
    {
      std::cerr << "Error: Cannot open file '" << outfile << "' for writing." << std::endl;
      exit(EXIT_FAILURE);
    }
  }
}
