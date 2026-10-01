#ifndef LINE_3D_H
#define LINE_3D_H

#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/curve3d.hpp>

using namespace godot;

class Line3D : public MeshInstance3D {
    GDCLASS(Line3D, MeshInstance3D)

private:
    // The curve that defines the shape of the line
    Ref<Curve3D> curve;
    // The radius of the line
    float radius = 0.1f;
    //Amount of segments to use for the line
    int segments = 16;
    //Resolution of how many vertices are used as an extrusion profile
    int resolution = 64;
    // Whether the line is capped at the ends
    bool capped = true;

    void generate_mesh();

    Ref<ArrayMesh> array_mesh;

protected:
    static void _bind_methods();

public:
    Line3D();
    ~Line3D();

    void set_curve(const Ref<Curve3D> &p_curve);
    Ref<Curve3D> get_curve() const;

    void set_radius(float p_radius);
    float get_radius() const;

    void set_capped(bool p_capped);
    bool is_capped() const;

    void set_segments(int p_segments);
    int get_segments() const;

    void set_resolution(int p_resolution);
    int get_resolution() const;
};

#endif