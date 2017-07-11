// #include <pdal/PointTable.hpp>
// #include <pdal/PointView.hpp>
#include <pdal/StageFactory.hpp>
// #include <pdal/Options.hpp>
#include <pdal/PipelineExecutor.hpp>
#include <pdal/DimUtil.hpp>

extern "C"
{
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/gprojects.h>
#include <grass/glocale.h>
}

#include <iostream>
#include <sstream>
#include <string>
#include <typeinfo>
#include "pipelinejson.hpp"

using namespace std;

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

    double xmin = 0;
    double ymin = 0;
    double xmax = 0;
    double ymax = 0;

    // // Set up the pipeline
    G_important_message(_("Setting up the PDAL pipeline..."));
    char* inFile = options->inFile->answer;
    char* outFile = options->outFile->answer;  /* */
    std::string pipeline_json =
            pipelineJson::basicVectorMapReaderWriter(inFile,outFile);

    cout << pipeline_json << endl;  //diagnostic only
    cout << endl;

    G_important_message(_("Running the pipeline ..."));
    auto pipeline = new pdal::PipelineExecutor(pipeline_json);
    //auto manager = new pdal::PipelineManager();

    cout << "is valid?  " << pipeline->validate() << endl;

    // // Start the reading process
    uint64_t pointCount = pipeline->execute();
    //cout << pointCount << endl;
    pdal::PipelineManager &mgr = pipeline->getManager();
    //pdal::FixedPointTable tbl = new pdal::FixedPointTable(mgr.pointTable());
    auto views = mgr.views();
    for (auto const& view : views){
        cout << pointCount << endl;
        cout << typeid(view).name() << endl;
        for (uint64_t idx = 0; idx < pointCount; ++idx) {
            //auto aPt = view->getPoint(idx);
            double x = view->getFieldAs<double>(pdal::Dimension::Id::X, idx);
            double y = view->getFieldAs<double>(pdal::Dimension::Id::Y, idx);
            double z = view->getFieldAs<double>(pdal::Dimension::Id::Z, idx);
            cout << x << ", " << y << ", " << z << endl;
        }
    }
    //auto variable = pdal::PointView(mgr.pointTable());
    cout << "done" << endl;
    return 0;
    //manager->addReader();
    //manager->execute();


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
