#include "Line3D.h"

using namespace godot;

void Line3D::_bind_methods() {
    // Bind the set_curve and get_curve methods to the Godot scripting API
    ClassDB::bind_method(
        D_METHOD("set_curve", "curve"),
        &Line3D::set_curve
    );

    ClassDB::bind_method(
        D_METHOD("get_curve"),
        &Line3D::get_curve
    );

    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "curve", PROPERTY_HINT_RESOURCE_TYPE, "Curve3D"),
        "set_curve",
        "get_curve"
    );


    // Bind the set_radius and get_radius methods to the Godot scripting API
    ClassDB::bind_method(
        D_METHOD("set_radius", "radius"),
        &Line3D::set_radius
    );

    ClassDB::bind_method(
        D_METHOD("get_radius"),
        &Line3D::get_radius
    );

    ADD_PROPERTY(
        PropertyInfo(Variant::FLOAT, 
        "radius",
        PROPERTY_HINT_RANGE,
        "0.0,100.0,0.01"),
        "set_radius",
        "get_radius"
    );

    // Bind the set_capped and is_capped methods to the Godot scripting API
    ClassDB::bind_method(
        D_METHOD("set_capped", "capped"),
        &Line3D::set_capped
    );

    ClassDB::bind_method(
        D_METHOD("is_capped"),
        &Line3D::is_capped
    );

    ADD_PROPERTY(
        PropertyInfo(Variant::BOOL, "capped"),
        "set_capped",
        "is_capped"
    );

    // Bind the set_segments and get_segments methods to the Godot scripting API
    ClassDB::bind_method(
        D_METHOD("set_segments", "segments"),
        &Line3D::set_segments
    );

    ClassDB::bind_method(
        D_METHOD("get_segments"),
        &Line3D::get_segments
    );

    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "segments", PROPERTY_HINT_RANGE, "1,100,1"),
        "set_segments",
        "get_segments"
    );

    // Bind the set_resolution and get_resolution methods to the Godot scripting API
    ClassDB::bind_method(
        D_METHOD("set_resolution", "resolution"),
        &Line3D::set_resolution
    );

    ClassDB::bind_method(
        D_METHOD("get_resolution"),
        &Line3D::get_resolution
    );

    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "resolution", PROPERTY_HINT_RANGE, "3,360,1"),
        "set_resolution",
        "get_resolution"
    );

    ClassDB::bind_method(
        D_METHOD("generate_mesh"),
        &Line3D::generate_mesh
    );
}

// Constructor and destructor
Line3D::Line3D() {
}

Line3D::~Line3D() {
}

//Generate the mesh based on the curve, radius, and capped properties
void Line3D::generate_mesh() {

    if (!curve.is_valid() || segments < 1 || resolution < 3 || radius <= 0.0f) {
        return;
    }

    // Clear the existing mesh and regenerate it from the current curve.
    if (!array_mesh.is_valid()) {
        array_mesh = Ref<ArrayMesh>(memnew(ArrayMesh));
        set_mesh(array_mesh);
    } else {
        array_mesh->clear_surfaces();
    }

    Array arrays;
    arrays.resize(Mesh::ARRAY_MAX);

    PackedVector3Array vertices;
    PackedInt32Array indices;
    PackedVector3Array normals;
    PackedVector2Array uvs;

    float total_length = curve->get_baked_length();
    float segment_length = total_length / float(segments);

    for (int i = 0; i <= segments; i++) {

        // 1. Get transform at this distance
        Transform3D ring_transform =
            curve->sample_baked_with_rotation(segment_length * float(i));

        // 2. Get center from transform
        Vector3 center = ring_transform.origin;

        // 3. Get two perpendicular directions
        //    from ring_transform.basis
        Vector3 circle_axis_1 = ring_transform.basis.get_column(0).normalized();
        Vector3 circle_axis_2 = ring_transform.basis.get_column(1).normalized();

        // 4. Generate resolution + 1 vertices
        //    The final vertex duplicates the first vertex so the UV can reach 1.0
        for (int j = 0; j <= resolution; j++) {

            // Calculate angle
            float angle = (float(j) / float(resolution)) * Math::TAU;

            // Calculate radial direction
            Vector3 radial =
                circle_axis_1 * Math::cos(angle) +
                circle_axis_2 * Math::sin(angle);

            // Add vertex
            vertices.push_back(center + radial * radius);

            // Add corresponding normal
            normals.push_back(radial.normalized());

            // Add UV coordinates
            // U goes from 0 -> 0.75 around the circumference
            // V goes from 0 -> 1 along the length of the line
            uvs.push_back(Vector2(
                float(j) / float(resolution) * 0.75,
                float(i) / float(segments)
            ));
        }
    }

    // 5. Generate indices for the triangles between rings
    for (int ring = 0; ring < segments; ring++) {
        for (int vertex = 0; vertex < resolution; vertex++) {

            // Current vertex on the current ring
            int current_ring_vertex =
                ring * (resolution + 1) + vertex;

            // Same vertex position on the next ring
            int next_ring_vertex =
                current_ring_vertex + (resolution + 1);

            // Next vertex around the current ring
            int current_ring_next_vertex =
                current_ring_vertex + 1;

            // Next vertex around the next ring
            int next_ring_next_vertex =
                current_ring_next_vertex + (resolution + 1);

            // First triangle
            indices.push_back(current_ring_vertex);
            indices.push_back(current_ring_next_vertex);
            indices.push_back(next_ring_vertex);

            // Second triangle
            indices.push_back(current_ring_next_vertex);
            indices.push_back(next_ring_next_vertex);
            indices.push_back(next_ring_vertex);
        }
    }

    // 6. If capped, generate the end caps
    if (capped) {
        // Generate the start cap
        Vector3 start_transform_origin =
            curve->sample_baked_with_rotation(0).origin;
        Vector3 start_normal =
            -curve->sample_baked_with_rotation(0).basis.get_column(2).normalized();

        int start_center_index = vertices.size();

        vertices.push_back(start_transform_origin);
        normals.push_back(start_normal);
        uvs.push_back(Vector2(0.5, 0.5));

        // The first ring contains resolution + 1 vertices.
        // The final vertex is a duplicate of vertex 0 for the UV seam,
        // so only use vertices 0 through resolution - 1 for the cap.
        for (int vertex = 0; vertex < resolution; vertex++) {
            int current_ring_vertex = vertex;
            int next_ring_vertex = vertex + 1;

            indices.push_back(start_center_index);
            indices.push_back(next_ring_vertex);
            indices.push_back(current_ring_vertex);
        }


        // Generate the end cap
        Vector3 end_transform_origin =
            curve->sample_baked_with_rotation(total_length).origin;
        Vector3 end_normal =
            curve->sample_baked_with_rotation(total_length).basis.get_column(2).normalized();

        int end_center_index = vertices.size();

        vertices.push_back(end_transform_origin);
        normals.push_back(end_normal);
        uvs.push_back(Vector2(0.5, 0.5));

        // Find the first vertex of the final ring.
        // Each ring now contains resolution + 1 vertices.
        int end_ring_start = segments * (resolution + 1);

        for (int vertex = 0; vertex < resolution; vertex++) {
            int current_ring_vertex = end_ring_start + vertex;
            int next_ring_vertex = end_ring_start + vertex + 1;

            indices.push_back(end_center_index);
            indices.push_back(current_ring_vertex);
            indices.push_back(next_ring_vertex);
        }
    }
    

    arrays[Mesh::ARRAY_VERTEX] = vertices;
    arrays[Mesh::ARRAY_INDEX] = indices;
    arrays[Mesh::ARRAY_NORMAL] = normals;
    arrays[Mesh::ARRAY_TEX_UV] = uvs;

    array_mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);
    
}

// Setters and getters
void Line3D::set_curve(const Ref<Curve3D> &p_curve) {
    // Disconnect from the old curve
    if (curve.is_valid()) {
        Callable callable = Callable(this, "generate_mesh");

        if (curve->is_connected("changed", callable)) {
            curve->disconnect("changed", callable);
        }
    }

    // Store the new curve
    curve = p_curve;

    // Connect to the new curve
    if (curve.is_valid()) {
        Callable callable = Callable(this, "generate_mesh");
        curve->connect("changed", callable);
    }

    // Rebuild the mesh
    generate_mesh();
}

Ref<Curve3D> Line3D::get_curve() const {
    return curve;
}

void Line3D::set_radius(float p_radius) {
    radius = p_radius;
    generate_mesh();
}

float Line3D::get_radius() const {
    return radius;
}

void Line3D::set_capped(bool p_capped) {
    capped = p_capped;
    generate_mesh();
}

bool Line3D::is_capped() const {
    return capped;
}

void Line3D::set_segments(int p_segments) {
    segments = p_segments;
    generate_mesh();
}

int Line3D::get_segments() const {
    return segments;
}

void Line3D::set_resolution(int p_resolution) {
    resolution = p_resolution;
    generate_mesh();
}

int Line3D::get_resolution() const {
    return resolution;
}