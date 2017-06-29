#include <pdal/PointTable.hpp>
#include <pdal/PointView.hpp>
#include <pdal/StageFactory.hpp>
#include <pdal/Options.hpp>

extern "C"
{
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/gprojects.h>
#include <grass/glocale.h>
}

extern "C"
{
#include "lidar.h"
#include "projection.h"
#include "filters.h"
}

#ifdef HAVE_LONG_LONG_INT
typedef unsigned long long gpoint_count;
#else
typedef unsigned long gpoint_count;
#endif

// // Note: Double Double Slashes are comments for Code Outline

typedef struct {
    Option *inFile;
    Option *outFile;
} CommandOptions;

CommandOptions* setup_all_options() {
    CommandOptions *allOpts = new CommandOptions();

    allOpts->inFile = G_define_standard_option(G_OPT_F_INPUT);
    allOpts->inFile->label = _("LAS input file");
    allOpts->inFile->description =
        _("LiDAR input files in LAS format (*.las or *.laz)");

    allOpts->outFile = G_define_standard_option(G_OPT_V_OUTPUT);

    return allOpts;
}

int main(int argc, char *argv[])
{
    // // Preliminaries
    G_gisinit(argv[0]);

    GModule *module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("import"));
    G_add_keyword(_("LIDAR"));
    G_add_keyword(_("PDAL"));
    module->description =
        _("Converts LAS LiDAR point clouds to a GRASS vector map using PDAL.");

    // // Set up Option* and Flag* values
    auto options = setup_all_options();

    // // Parse command line arguments
    if (G_parser(argc, argv))
        return EXIT_FAILURE;

    if (access(options->inFile->answer, F_OK) != 0) {
        G_fatal_error(_("Input file <%s> does not exist"), options->inFile->answer);
    }

    // // Unknown section
    // we use full qualification because the dim ns contains too general names

    struct GLidarLayers layers;
    GLidarLayers_set_no_layers(&layers);
    layers.id_layer = 1;



    double xmin = 0;
    double ymin = 0;
    double xmax = 0;
    double ymax = 0;

    // // Prepare PDAL to read input file
    pdal::StageFactory factory;
    std::string pdal_read_driver = factory.inferReaderDriver(options->inFile->answer);
    if (pdal_read_driver.empty())
        G_fatal_error("Cannot determine input file type of <%s>",
                      options->inFile->answer);

    pdal::Option las_opt("filename", options->inFile->answer);
    pdal::Options las_opts;
    las_opts.add(las_opt);
    // TODO: free reader
    // using plain pointer because we need to keep the last stage pointer
    pdal::Stage * reader = factory.createStage(pdal_read_driver);
    if (!reader)
        G_fatal_error("PDAL reader creation failed, a wrong format of <%s>",
                      options->inFile->answer);
    reader->setOptions(las_opts);

    pdal::PointTable point_table;


    // // Start the reading process
    G_important_message(_("Running PDAL algorithms..."));


    // // Create output Vector Map
    G_important_message(_("Scanning points..."));
    struct Map_info output_vector;

    // the overwrite warning comes quite late in the execution
    // but that's good enough
    if (Vect_open_new(&output_vector, options->outFile->answer, 1) < 0)
        G_fatal_error(_("Unable to create vector map <%s>"), options->outFile->answer);
    Vect_hist_command(&output_vector);


    struct line_pnts *points = Vect_new_line_struct();
    struct line_cats *cats = Vect_new_cats_struct();

    int cat = 1;
    bool cat_max_reached = false;

    // // Main Processing Loop

    // not building topology by default
    Vect_close(&output_vector);
}
