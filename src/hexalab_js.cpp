#include <app.h>
#include <plane_filter.h>
#include <quality_filter.h>
#include <peeling_filter.h>
#include <color_map.h>
#include <hex_quality.h>

using namespace HexaLab;
using namespace Eigen;

template<typename T>
js_ptr buffer_data(std::vector<T>& v) {
    return (js_ptr)v.data();
}
template<typename T>
size_t buffer_size(std::vector<T>& v) {
    return v.size();
}

float mesh_stats_aabb_diag(MeshStats& stats) {
    return stats.aabb.diagonal().norm();
}
Vector3f mesh_stats_aabb_size(MeshStats& stats) {
    return stats.aabb.sizes();
}
Vector3f mesh_stats_center(MeshStats& stats) {
    return stats.aabb.center();
}

//vector<float>* hexa_quality(App& app) {
//    return &app.get_hexa_quality();
//}

vector<Vector3f>* get_surface_vert_pos(Model& model) { return &model.surface_vert_pos; }
vector<Vector3f>* get_surface_vert_norm(Model& model) { return &model.surface_vert_norm; }
vector<Vector3f>* get_surface_vert_color(Model& model) { return &model.surface_vert_color; }
vector<Vector3f>* get_wireframe_vert_pos(Model& model) { return &model.wireframe_vert_pos; }
vector<Vector3f>* get_wireframe_vert_color(Model& model) { return &model.wireframe_vert_color; }

void set_color_map(App& app, ColorMap::Palette palette) {
    app.color_map = ColorMap(palette);
}

Vector3f map_to_color(App& app, float value) {
    return app.color_map.get(value);
}

enum class QualityMeasure {
    ScaledJacobian,
    DiagonalRatio,
    EdgeRatio
};

void compute_hexa_quality(App& app, QualityMeasure measure) {
    App::quality_measure_fun* fun;
    switch(measure) {
    case QualityMeasure::ScaledJacobian:
        fun = &scaled_jacobian;
        break;
    case QualityMeasure::DiagonalRatio:
        fun = &diagonal_ratio;
        break;
    case QualityMeasure::EdgeRatio:
        fun = &edge_ratio;
        break;
    }
    app.compute_hexa_quality(fun);
}

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/bind.h>
using namespace emscripten;
EMSCRIPTEN_BINDINGS(HexaLab) {

    class_<App>("App")
        .constructor<>()
        .function("build_models",                   &App::build_surface_models)
        .function("import_mesh",                    &App::import_mesh)
        .function("add_filter",                     &App::add_filter, allow_raw_pointers())
        .function("get_visible_model",              &App::get_visible_model, allow_raw_pointers())
        .function("get_filtered_model",             &App::get_filtered_model, allow_raw_pointers())
        .function("get_singularity_model",          &App::get_singularity_model, allow_raw_pointers())
        .function("get_boundary_singularity_model", &App::get_boundary_singularity_model, allow_raw_pointers())
        .function("get_boundary_creases_model",     &App::get_boundary_creases_model, allow_raw_pointers())
        .function("get_hexa_quality",               &App::get_hexa_quality, allow_raw_pointers())
        .function("set_color_map",                  &set_color_map)
        .function("map_to_color",                   &map_to_color)
        .function("get_mesh",                       &App::get_mesh_stats, allow_raw_pointers())
        .function("compute_hexa_quality",           &compute_hexa_quality)
        .function("set_visible_outside_color",      &App::set_visible_outside_color)
        .function("set_visible_inside_color",       &App::set_visible_inside_color)
        .function("get_visible_outside_color",      &App::get_visible_outside_color)
        .function("get_visible_inside_color",       &App::get_visible_inside_color)
        .property("do_show_color_map",              &App::do_show_color_map)
        ;

    enum_<ColorMap::Palette>("ColorMap")
        .value("Parula",                    ColorMap::Palette::Parula)
        .value("Jet",                       ColorMap::Palette::Jet)
        .value("RedGreen",                  ColorMap::Palette::RedGreen)
        ;

    enum_<QualityFilter::Operator>("QualityFilterOperator")
        .value("Inside",                    QualityFilter::Operator::Inside)
        .value("Outside",                   QualityFilter::Operator::Outside)
        ;

    enum_<QualityMeasure>("QualityMeasure")
        .value("ScaledJacobian",            QualityMeasure::ScaledJacobian)
        .value("DiagonalRatio",             QualityMeasure::DiagonalRatio)
        .value("EdgeRatio",                 QualityMeasure::EdgeRatio)
        ;

    class_<Model>("Model")
        .constructor<>()
        .function("surface_pos",            &get_surface_vert_pos, allow_raw_pointers())
        .function("surface_norm",           &get_surface_vert_norm, allow_raw_pointers())
        .function("surface_color",          &get_surface_vert_color, allow_raw_pointers())
        .function("wireframe_pos",          &get_wireframe_vert_pos, allow_raw_pointers())
        .function("wireframe_color",        &get_wireframe_vert_color, allow_raw_pointers())
        ;
/*
    class_<Mesh>("Mesh")
        .constructor<>()
        .function("get_aabb_diag",          &mesh_aabb_diag)
        .function("get_aabb_size",          &mesh_aabb_size)
        .function("get_aabb_center",        &mesh_center)
        ;
*/
    class_<IFilter>("Filter")
        .property("enabled",                &IFilter::enabled)
        ;

    class_<PlaneFilter, base<IFilter>>("PlaneFilter")
        .constructor<>()
        .function("filter",                 &PlaneFilter::filter)
        .function("on_mesh_set",            &PlaneFilter::on_mesh_set)
        .function("set_plane_normal",       &PlaneFilter::set_plane_normal)
        .function("set_plane_offset",       &PlaneFilter::set_plane_offset)
        .function("get_plane_normal",       &PlaneFilter::get_plane_normal)
        .function("get_plane_offset",       &PlaneFilter::get_plane_offset)
        .function("get_plane_world_offset", &PlaneFilter::get_plane_world_offset)
        ;

    class_<QualityFilter, base<IFilter>>("QualityFilter")
        .constructor<>()
        .function("filter",                 &QualityFilter::filter)
        .function("on_mesh_set",            static_cast<void(QualityFilter::*)(Mesh&)>(&IFilter::on_mesh_set))
        .property("quality_threshold_min",  &QualityFilter::quality_threshold_min)
        .property("quality_threshold_max",  &QualityFilter::quality_threshold_max)
        .property("operator",               &QualityFilter::op)
        ;

    class_<PeelingFilter, base<IFilter>>("PeelingFilter")
        .constructor<>()
        .function("filter",                 &PeelingFilter::filter)
        .function("on_mesh_set",            static_cast<void(PeelingFilter::*)(Mesh&)>(&IFilter::on_mesh_set))
        .property("peeling_depth",          &PeelingFilter::depth_threshold)
        .property("max_depth",              &PeelingFilter::max_depth)
        ;

    class_<MeshStats>("Mesh")
        .property("vert_count",             &MeshStats::vert_count)
        .property("hexa_count",             &MeshStats::hexa_count)
        .property("min_edge_len",           &MeshStats::min_edge_len)
        .property("max_edge_len",           &MeshStats::max_edge_len)
        .property("avg_edge_len",           &MeshStats::avg_edge_len)
        .property("quality_min",            &MeshStats::quality_min)
        .property("quality_max",            &MeshStats::quality_max)
        .property("quality_avg",            &MeshStats::quality_avg)
        .property("quality_var",            &MeshStats::quality_var)
        .function("get_aabb_diagonal",      &mesh_stats_aabb_diag)
        .function("get_aabb_size",          &mesh_stats_aabb_size)
        .function("get_aabb_center",        &mesh_stats_center)
        ;

    class_<Vector3f>("float3")
        .constructor<>()
        .function("x",                      static_cast<float&(Vector3f::*)()>(select_overload<float&()>(&Vector3f::x)))
        .function("y",                      static_cast<float&(Vector3f::*)()>(select_overload<float&()>(&Vector3f::y)))
        .function("z",                      static_cast<float&(Vector3f::*)()>(select_overload<float&()>(&Vector3f::z)))
        ;

    class_<vector<Vector3f>>("buffer3f")
        .constructor<>()
        .function("data",                   &buffer_data<Vector3f>)
        .function("size",                   &buffer_size<Vector3f>)
        ;

    class_<vector<float>>("buffer1f")
        .constructor<>()
        .function("data",                   &buffer_data<float>)
        .function("size",                   &buffer_size<float>)
        ;

}
#endif
