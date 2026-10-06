#ifndef LINE_3D_H
#define LINE_3D_H

#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/curve3d.hpp>
#include <godot_cpp/classes/curve.hpp>
#include <godot_cpp/core/binder_common.hpp>
#include <godot_cpp/classes/path3d.hpp>

using namespace godot;

class Line3D : public MeshInstance3D {
    GDCLASS(Line3D, MeshInstance3D)

public:
    enum CapMode {
        CAP_NONE,
        CAP_FLAT,
        CAP_CONE,
        CAP_ROUND
    };

private:
    //The curve that defines the shape of the line
    Ref<Curve3D> curve;
    //The width of the line
    float width = 0.1f;
    //Width Curve
    Ref<Curve> width_curve;
    //Amount of segments to use for the line
    int segments = 16;
    //Resolution of how many vertices are used as an extrusion profile
    int resolution = 64;
    // Cap mode enums

    CapMode cap_mode = CAP_FLAT;

    void generate_mesh();

    void try_assign_parent_curve();

    void _notification(int p_what);

protected:
    static void _bind_methods();

public:
    Line3D();
    ~Line3D();

    void set_curve(const Ref<Curve3D> &p_curve);
    Ref<Curve3D> get_curve() const;

    void set_width(float p_width);
    float get_width() const;

    void set_width_curve(const Ref<Curve> &p_width_curve);
    Ref<Curve> get_width_curve() const;

    void set_cap_mode(CapMode p_cap_mode);
    CapMode get_cap_mode() const;

    void set_segments(int p_segments);
    int get_segments() const;

    void set_resolution(int p_resolution);
    int get_resolution() const;
};

VARIANT_ENUM_CAST(Line3D::CapMode)

#endif