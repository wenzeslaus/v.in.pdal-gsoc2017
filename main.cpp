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

    Option *id_layer_opt = G_define_standard_option(G_OPT_V_FIELD);
    id_layer_opt->key = "id_layer";
    id_layer_opt->label = _("Layer number to store generated point ID as category");
    id_layer_opt->description = _("Set to 1 by default, use -c to not store it");
    id_layer_opt->answer = NULL;
    id_layer_opt->guisection = _("Categories");

    Option *return_layer_opt = G_define_standard_option(G_OPT_V_FIELD);
    return_layer_opt->key = "return_layer";
    return_layer_opt->label =
        _("Layer number to store return information as category");
    return_layer_opt->description = _("Leave empty to not store it");
    return_layer_opt->answer = NULL;
    return_layer_opt->guisection = _("Categories");

    Option *class_layer_opt = G_define_standard_option(G_OPT_V_FIELD);
    class_layer_opt->key = "class_layer";
    class_layer_opt->label =
        _("Layer number to store class number as category");
    class_layer_opt->description = _("Leave empty to not store it");
    class_layer_opt->answer = NULL;
    class_layer_opt->guisection = _("Categories");

    Option *rgb_layer_opt = G_define_standard_option(G_OPT_V_FIELD);
    rgb_layer_opt->key = "rgb_layer";
    rgb_layer_opt->label =
        _("Layer number where RBG colors are stored as category");
    rgb_layer_opt->description = _("Leave empty to not store it");
    rgb_layer_opt->answer = NULL;
    rgb_layer_opt->guisection = _("Categories");

    Option *spatial_opt = G_define_option();
    spatial_opt->key = "spatial";
    spatial_opt->type = TYPE_DOUBLE;
    spatial_opt->multiple = NO;
    spatial_opt->required = NO;
    // TODO: does this require multiple or not?
    spatial_opt->key_desc = "xmin,ymin,xmax,ymax";
    spatial_opt->label = _("Import subregion only");
    spatial_opt->description =
        _("Format: xmin,ymin,xmax,ymax - usually W,S,E,N");
    spatial_opt->guisection = _("Selection");

    Option *zrange_opt = G_define_option();
    zrange_opt->key = "zrange";
    zrange_opt->type = TYPE_DOUBLE;
    zrange_opt->required = NO;
    zrange_opt->key_desc = "min,max";
    zrange_opt->description = _("Filter range for z data (min,max)");
    zrange_opt->guisection = _("Selection");

    Option *filter_opt = G_define_option();
    filter_opt->key = "return_filter";
    filter_opt->type = TYPE_STRING;
    filter_opt->required = NO;
    filter_opt->label = _("Only import points of selected return type");
    filter_opt->description = _("If not specified, all points are imported");
    filter_opt->options = "first,last,mid";
    filter_opt->guisection = _("Selection");

    Option *class_opt = G_define_option();
    class_opt->key = "class_filter";
    class_opt->type = TYPE_INTEGER;
    class_opt->multiple = YES;
    class_opt->required = NO;
    class_opt->label = _("Only import points of selected class(es)");
    class_opt->description = _("Input is comma separated integers. "
                               "If not specified, all points are imported.");
    class_opt->guisection = _("Selection");

    Flag *reproject_flag = G_define_flag();
    reproject_flag->key = 'w';
    reproject_flag->label =
        _("Reproject to location's coordinate system if needed");
    reproject_flag->description =
        _("Reprojects input dataset to the coordinate system of"
          " the GRASS location (by default only datasets with the"
          " matching cordinate system can be imported");
    reproject_flag->guisection = _("Projection");

    Flag *over_flag = G_define_flag();
    over_flag->key = 'o';
    over_flag->label =
        _("Override projection check (use current location's projection)");
    over_flag->description =
        _("Assume that the dataset has same projection as the current location");
    over_flag->guisection = _("Projection");

    // TODO: from the API it seems that also prj file path and proj string will work
    Option *input_srs_opt = G_define_option();
    input_srs_opt->key = "input_srs";
    input_srs_opt->type = TYPE_STRING;
    input_srs_opt->required = NO;
    input_srs_opt->label =
            _("Input dataset projection (WKT or EPSG, e.g. EPSG:4326)");
    input_srs_opt->description =
            _("Override input dataset coordinate system using EPSG code"
              " or WKT definition");
    input_srs_opt->guisection = _("Projection");

    Option *max_ground_window_opt = G_define_option();
    max_ground_window_opt->key = "max_ground_window_size";
    max_ground_window_opt->type = TYPE_DOUBLE;
    max_ground_window_opt->required = NO;
    max_ground_window_opt->answer = "33";
    max_ground_window_opt->description =
        _("Maximum window size for ground filter");
    max_ground_window_opt->guisection = _("Ground filter");

    Option *ground_slope_opt = G_define_option();
    ground_slope_opt->key = "ground_slope";
    ground_slope_opt->type = TYPE_DOUBLE;
    ground_slope_opt->required = NO;
    ground_slope_opt->answer = "1.0";
    ground_slope_opt->description = _("Slope for ground filter");
    ground_slope_opt->guisection = _("Ground filter");

    Option *max_ground_distance_opt = G_define_option();
    max_ground_distance_opt->key = "max_ground_distance";
    max_ground_distance_opt->type = TYPE_DOUBLE;
    max_ground_distance_opt->required = NO;
    max_ground_distance_opt->answer = "2.5";
    max_ground_distance_opt->description =
        _("Maximum distance for ground filter");
    max_ground_distance_opt->guisection = _("Ground filter");

    Option *init_ground_distance_opt = G_define_option();
    init_ground_distance_opt->key = "initial_ground_distance";
    init_ground_distance_opt->type = TYPE_DOUBLE;
    init_ground_distance_opt->required = NO;
    init_ground_distance_opt->answer = "0.15";
    init_ground_distance_opt->description =
        _("Initial distance for ground filter");
    init_ground_distance_opt->guisection = _("Ground filter");

    Option *ground_cell_size_opt = G_define_option();
    ground_cell_size_opt->key = "ground_cell_size";
    ground_cell_size_opt->type = TYPE_DOUBLE;
    ground_cell_size_opt->required = NO;
    ground_cell_size_opt->answer = "1";
    ground_cell_size_opt->description =
        _("Initial distance for ground filter");
    ground_cell_size_opt->guisection = _("Ground filter");

    Flag *nocats_flag = G_define_flag();
    nocats_flag->key = 'c';
    nocats_flag->label =
        _("Do not automatically add unique ID as category to each point");
    nocats_flag->description =
        _("Create only requested layers and categories");
    /* v.in.lidar has this in Speed but we don't have it here */
    nocats_flag->guisection = _("Categories");

    Flag *region_flag = G_define_flag();
    region_flag->key = 'r';
    region_flag->guisection = _("Selection");
    region_flag->description = _("Limit import to the current region");

    Flag *extract_ground_flag = G_define_flag();
    extract_ground_flag->key = 'j';
    extract_ground_flag->label =
        _("Classify and extract ground points");
    extract_ground_flag->description =
        _("This assignes class 2 to the groud points");
    extract_ground_flag->guisection = _("Ground filter");

    // TODO: by inverting class filter and choosing 2 we can select non-groud points
    // this can be done as a separate flag (generally useful?)
    // or this flag can be changed (only ground is classified anyway)
    // and it would Classify ground and extract non-ground
    // probably better if only one step is required to get ground and non-ground
    Flag *classify_ground_flag = G_define_flag();
    classify_ground_flag->key = 'k';
    classify_ground_flag->description = _("Classify ground points");
    classify_ground_flag->guisection = _("Ground filter");

    Flag *height_filter_flag = G_define_flag();
    height_filter_flag->key = 'h';
    height_filter_flag->label =
        _("Compute height for points as a difference from ground");
    height_filter_flag->description =
        _("This requires points to have class 2");
    height_filter_flag->guisection = _("Transform");

    Flag *approx_ground_flag = G_define_flag();
    approx_ground_flag->key = 'm';
    approx_ground_flag->description =
        _("Use approximate algorithm in ground filter");
    approx_ground_flag->guisection = _("Ground filter");

    G_option_exclusive(spatial_opt, region_flag, NULL);
    G_option_exclusive(reproject_flag, over_flag, NULL);
    G_option_exclusive(extract_ground_flag, classify_ground_flag, NULL);
    G_option_exclusive(nocats_flag, id_layer_opt, NULL);
    G_option_requires(return_layer_opt, id_layer_opt, nocats_flag, NULL);
    G_option_requires(class_layer_opt, id_layer_opt, nocats_flag, NULL);
    G_option_requires(rgb_layer_opt, id_layer_opt, nocats_flag, NULL);

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
    if (id_layer_opt->answer && id_layer_opt->answer[0] != '\0')
        layers.id_layer = std::stoi(id_layer_opt->answer);
    if (return_layer_opt->answer && return_layer_opt->answer[0] != '\0')
        layers.return_layer = std::stoi(return_layer_opt->answer);
    if (class_layer_opt->answer && class_layer_opt->answer[0] != '\0')
        layers.class_layer = std::stoi(class_layer_opt->answer);
    if (rgb_layer_opt->answer && rgb_layer_opt->answer[0] != '\0')
        layers.rgb_layer = std::stoi(rgb_layer_opt->answer);

    if (nocats_flag->answer) {
        layers.id_layer = 0;
    }

    /* this is plain C but in sync with v.in.lidar */
    Option *layer_options[4] = {id_layer_opt, return_layer_opt,
                                class_layer_opt, rgb_layer_opt};
    int layer_values[4] = {layers.id_layer, layers.return_layer,
                           layers.class_layer, layers.rgb_layer};

    if (layers.id_layer)
        G_verbose_message(_("Storing generated point IDs as categories"
                            " in the layer <%d>, consequently no more"
                            " than %d points can be imported"),
                          layers.id_layer, GV_CAT_MAX);

    double xmin = 0;
    double ymin = 0;
    double xmax = 0;
    double ymax = 0;
    bool use_spatial_filter = false;
    if (spatial_opt->answer)
        use_spatial_filter = spatial_filter_from_option(spatial_opt,
                                                        &xmin, &ymin,
                                                        &xmax, &ymax);
    else if (region_flag->answer)
        use_spatial_filter = spatial_filter_from_current_region(&xmin,
                                                                &ymin,
                                                                &xmax,
                                                                &ymax);

    double zrange_min, zrange_max;
    bool use_zrange = zrange_filter_from_option(zrange_opt, &zrange_min,
                                                &zrange_max);
    struct ReturnFilter return_filter_struct;
    bool use_return_filter =
        return_filter_create_from_string(&return_filter_struct,
                                         filter_opt->answer);
    struct ClassFilter class_filter;
    bool use_class_filter =
        class_filter_create_from_strings(&class_filter, class_opt->answers);

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

    // getting projection is possible only after prepare
    if (over_flag->answer) {
        G_important_message(_("Overriding projection check and assuming"
                              " that the projection of input matches"
                              " the location projection"));
    }
    else if (!reproject_flag->answer) {  /* * /
        pdal::SpatialReference spatial_reference = reader->getSpatialReference();
        if (spatial_reference.empty())
            G_fatal_error(_("The input dataset has undefined projection"));
        std::string dataset_wkt =
            spatial_reference.
            getWKT(pdal::SpatialReference::eHorizontalOnly);
        bool proj_match = is_wkt_projection_same_as_loc(dataset_wkt.c_str());
        if (!proj_match)
            wkt_projection_mismatch_report(dataset_wkt.c_str());  /* */
    }

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
