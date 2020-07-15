FILE* input1 = stdin;
FILE* input2 = stdin;
FILE* output = stdout;
{
  string infile1;
  string infile2;
  string outfile;
  switch(argc - optind)
  {
    case 0:
      break;

    case 1:
      infile1 = argv[argc-1];
      break;

    case 2:
      infile1 = argv[argc-2];
      infile2 = argv[argc-1];
      break;

    case 3:
      infile1 = argv[argc-3];
      infile2 = argv[argc-2];
      outfile = argv[argc-1];
      break;

    default:
      endProgram(argv[0]);
      break;
  }

  if(infile1 != "" && infile1 != "-")
  {
    input1 = fopen(infile1.c_str(), "rb");
    if(!input1)
    {
      std::cerr << "Error: Cannot open file '" << infile1 << "' for reading." << std::endl;
      exit(EXIT_FAILURE);
    }
  }

  if(infile2 != "" && infile2 != "-")
  {
    input2 = fopen(infile2.c_str(), "rb");
    if(!input2)
    {
      std::cerr << "Error: Cannot open file '" << infile2 << "' for reading." << std::endl;
      exit(EXIT_FAILURE);
    }
  }

  if(outfile != "" && outfile != "-")
  {
    output = fopen(outfile.c_str(), "wb");
    if(!output)
    {
      std::cerr << "Error: Cannot open file '" << outfile << "' for writing." << std::endl;
      exit(EXIT_FAILURE);
    }
  }
}
