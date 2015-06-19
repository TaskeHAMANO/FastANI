#include "CommandSketch.h"
#include "Sketch.h"
#include <iostream>

using namespace::std;

CommandSketch::CommandSketch()
: Command()
{
    name = "sketch";
    description = "Create a sketch, which is a reduced representation of a sequence or set of sequences (based on min-hashes) that can be used for fast distance calculations. Input can be a fasta or fastq files (gzipped or not), and \"-\" can be given to read from standard input. By default, the output will be the input file with a '.msh' extension, or 'stdin.msh' if standard input is used (see -o).";
    argumentString = "fast(a|q)[.gz] ...";
    
    useOption("help");
    useOption("kmer");
    //useOption("windowed");
    //useOption("window");
    useOption("sketchSize");
    //useOption("verbose");
    //useOption("silent");
    useOption("concat");
    useOption("unique");
    useOption("genome");
    useOption("memory");
    useOption("bloomError");
    //useOption("illumina");
    //useOption("pacbio");
    //useOption("nanopore");
    useOption("noncanonical");
    addOption("prefix", Option(Option::File, "o", "Output prefix (first input file used if unspecified). The suffix '.msh' will be appended.", ""));
    addOption("list", Option(Option::Boolean, "l", "Input files are lists of file names.", ""));
}

int CommandSketch::run() const
{
    if ( arguments.size() == 0 || options.at("help").active )
    {
        print();
        return 0;
    }
    
    int verbosity = 0;//options.at("silent").active ? 0 : options.at("verbose").active ? 2 : 1;
    bool list = options.at("list").active;
    
    Sketch::Parameters parameters;
    
    parameters.kmerSize = options.at("kmer").getArgumentAsNumber();
    parameters.minHashesPerWindow = options.at("sketchSize").getArgumentAsNumber();
    parameters.concatenated = options.at("concat").active;
    parameters.noncanonical = options.at("noncanonical").active;
    parameters.bloomFilter = options.at("unique").active;
    parameters.genomeSize = options.at("genome").getArgumentAsNumber();
    parameters.memoryMax = options.at("memory").getArgumentAsNumber();
    parameters.bloomError = options.at("bloomError").getArgumentAsNumber();
    parameters.windowed = false;//options.at("windowed").active;
    parameters.windowSize = 0;//options.at("window").getArgumentAsNumber();
    
    if ( options.at("genome").active || options.at("memory").active )
    {
        parameters.bloomFilter = true;
    }
    
    if ( parameters.bloomFilter )
    {
        parameters.concatenated = true;
    }
    
    if ( parameters.concatenated && parameters.windowed )
    {
        cerr << "ERROR: " << options.at("concat").identifier << " and " << options.at("windowed").identifier << " are incompatible." << endl;
        return 1;
    }
    
    for ( int i = 0; i < arguments.size(); i++ )
    {
        if ( hasSuffix(arguments[i], suffixSketch) )
        {
            cerr << "ERROR: " << arguments[i] << " looks like it is already sketched.\n";
            exit(1);
        }
    }
    
    Sketch sketch;
    
    vector<string> files;
    
    for ( int i = 0; i < arguments.size(); i++ )
    {
        if ( list )
        {
            splitFile(arguments[i], files);
        }
        else
        {
            files.push_back(arguments[i]);
        }
    }
    
    sketch.initFromSequence(files, parameters, verbosity);
    
    string prefix;
    
    if ( options.at("prefix").argument.length() > 0 )
    {
        prefix = options.at("prefix").argument;
    }
    else
    {
        if ( arguments[0] == "-" )
        {
            prefix = "stdin";
        }
        else
        {
            prefix = arguments[0];
        }
    }
    
    string suffix = parameters.windowed ? suffixSketchWindowed : suffixSketch;
    
    if ( ! hasSuffix(prefix, suffix) )
    {
        prefix += suffix;
    }
    
    sketch.writeToCapnp(prefix.c_str());
    
    return 0;
}