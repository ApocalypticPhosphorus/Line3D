#include "Line3D.h"

using namespace godot;

void Line3D::_bind_methods() {

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


    ClassDB::bind_method(
        D_METHOD("set_width", "width"),
        &Line3D::set_width
    );

    ClassDB::bind_method(
        D_METHOD("get_width"),
        &Line3D::get_width
    );

    ADD_PROPERTY(
        PropertyInfo(Variant::FLOAT, 
        "width",
        PROPERTY_HINT_RANGE,
        "0.0,100.0,0.01"),
        "set_width",
        "get_width"
    );

    ClassDB::bind_method(
        D_METHOD("set_width_curve", "width_curve"),
        &Line3D::set_width_curve
    );

    ClassDB::bind_method(
        D_METHOD("get_width_curve"),
        &Line3D::get_width_curve
    );

    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "width_curve", PROPERTY_HINT_RESOURCE_TYPE, "Curve"),
        "set_width_curve",
        "get_width_curve"
    );

    ClassDB::bind_method(
        D_METHOD("set_cap_mode", "cap_mode"),
        &Line3D::set_cap_mode
    );

    ClassDB::bind_method(
        D_METHOD("get_cap_mode"),
        &Line3D::get_cap_mode
    );

    BIND_ENUM_CONSTANT(CAP_NONE);
    BIND_ENUM_CONSTANT(CAP_FLAT);
    BIND_ENUM_CONSTANT(CAP_CONE);
    BIND_ENUM_CONSTANT(CAP_ROUND);

    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "cap_mode", PROPERTY_HINT_ENUM, "None,Flat,Cone,Round"),
        "set_cap_mode",
        "get_cap_mode"
    );

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

void Line3D::generate_mesh() {

    if (!curve.is_valid() || segments < 1 || resolution < 3 || width <= 0.0f) {
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

    float radius = width / 2.0f;

    float total_length = curve->get_baked_length();
    float segment_length = total_length / float(segments);

    // Generate segments
    for (int i = 0; i <= segments; i++) {

        // 1. Get transform at this distance
        Transform3D ring_transform = curve->sample_baked_with_rotation(segment_length * float(i));

        // 2. Get center from transform
        Vector3 center = ring_transform.origin;

        // 3. Get two perpendicular directions from ring_transform.basis
        Vector3 circle_axis_1 = ring_transform.basis.get_column(0).normalized();
        Vector3 circle_axis_2 = ring_transform.basis.get_column(1).normalized();

        // Get width from width_curve if it exists, from 0 at the start to 1 at the end of the curve. If width_curve is not valid, use the default width.
        float segment_width = 1.0f; // Default width
        if (width_curve.is_valid()) {
            segment_width = width_curve->sample(float(i) / float(segments));
        }

        // 4. Generate resolution + 1 vertices. The final vertex duplicates the first vertex so the UV can reach 1.0
        for (int j = 0; j <= resolution; j++) {

            // Calculate angle
            float angle = (float(j) / float(resolution)) * Math::TAU;

            // Calculate radial direction
            Vector3 radial =
                circle_axis_1 * Math::cos(angle) +
                circle_axis_2 * Math::sin(angle);

            // Add vertex
            vertices.push_back(center + radial * width / 2.0f * segment_width);

            // Add corresponding normal
            normals.push_back(radial.normalized());

            // Add UV coordinates
            // U goes from 0 to 0.75 around the circumference
            // V goes from 0 to 1 along the length of the line
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
            int current_ring_vertex = ring * (resolution + 1) + vertex;

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
    if (cap_mode == CAP_FLAT || cap_mode == CAP_CONE) {
        // Generate the start cap

        // The circle uvs should be a circle of diameter 0.25 with a center at (0.875, 0.125) in uv space. The normals should be the negative of the tangent at the start of the curve.
        Transform3D start_transform = curve->sample_baked_with_rotation(0);
        Vector3 start_transform_origin = start_transform.origin;
        Vector3 start_normal = -start_transform.basis.get_column(2).normalized();

        int start_center_index = vertices.size();

        float start_segment_width = 1.0f;
        if (width_curve.is_valid()) {
            start_segment_width = width_curve->sample(0.0f);
        }

        // If the cap mode is cone, we need to add a vertex at the center of the start cap that is offset along the normal by the width of the line by half of the width. This will create a cone shape for the start cap.
        if (cap_mode == CAP_CONE) {
            vertices.push_back(start_transform_origin - start_normal * radius * start_segment_width);
        } else {
            vertices.push_back(start_transform_origin);
        }

        normals.push_back(start_normal);
        uvs.push_back(Vector2(0.875, 0.125));

        // Generate first ring of vertices for the start cap. The first ring contains resolution + 1 vertices, with the final vertex being a duplicate of vertex 0 for the UV seam.
        Vector3 start_circle_axis_1 = start_transform.basis.get_column(0).normalized();
        Vector3 start_circle_axis_2 = start_transform.basis.get_column(1).normalized();

        for (int vertex = 0; vertex <= resolution; vertex++) {
            float angle = (float(vertex) / float(resolution)) * Math::TAU;
            Vector3 radial = start_circle_axis_1 * Math::cos(angle) + start_circle_axis_2 * Math::sin(angle);
            vertices.push_back(start_transform_origin + radial * radius * start_segment_width);
            if (cap_mode == CAP_CONE) {
                normals.push_back((radial - start_normal).normalized());
            } else {
                normals.push_back(start_normal);
            }
            // Transform the radial vector into local space
            uvs.push_back(Vector2(
                0.875 + Math::cos(angle) * 0.125,
                0.125 - Math::sin(angle) * 0.125
            ));
        }

        // Generate the triangles for the start cap. Each triangle connects the center vertex to two adjacent vertices on the new first ring.
        for (int vertex = 0; vertex < resolution; vertex++) {
            int current_ring_vertex = start_center_index + 1 + vertex;
            int next_ring_vertex = start_center_index + 1 + vertex + 1;

            indices.push_back(start_center_index);
            indices.push_back(next_ring_vertex);
            indices.push_back(current_ring_vertex);
        }


        // Generate the end cap
        Transform3D end_transform = curve->sample_baked_with_rotation(total_length);
        Vector3 end_transform_origin = end_transform.origin;
        Vector3 end_normal = end_transform.basis.get_column(2).normalized();

        int end_center_index = vertices.size();

        float end_segment_width = 1.0f;
        if (width_curve.is_valid()) {
            end_segment_width = width_curve->sample(1.0f);
        }

        if(cap_mode == CAP_CONE) {
            vertices.push_back(end_transform_origin - end_normal * radius * end_segment_width);
        } else {
            vertices.push_back(end_transform_origin);
        }

        normals.push_back(end_normal);
        uvs.push_back(Vector2(0.875, 0.375));

        // Generate first ring of vertices for the end cap. The first ring contains resolution + 1 vertices, with the final vertex being a duplicate of vertex 0 for the UV seam.
        Vector3 end_circle_axis_1 = end_transform.basis.get_column(0).normalized();
        Vector3 end_circle_axis_2 = end_transform.basis.get_column(1).normalized();

        for (int vertex = 0; vertex <= resolution; vertex++) {
            float angle = (float(vertex) / float(resolution)) * Math::TAU;
            Vector3 radial = end_circle_axis_1 * Math::cos(angle) + end_circle_axis_2 * Math::sin(angle);
            vertices.push_back(end_transform_origin + radial * radius * end_segment_width);
            if(cap_mode == CAP_CONE) {
                normals.push_back((radial - end_normal).normalized() );
            } else {
                normals.push_back(end_normal);
            }
            uvs.push_back(Vector2(
                0.875 - Math::cos(angle) * 0.125,
                0.375 - Math::sin(angle) * 0.125
            ));
        }

        // Generate the triangles for the end cap. Each triangle connects the center vertex to two adjacent vertices on the new first ring.
        for (int vertex = 0; vertex < resolution; vertex++) {
            int current_ring_vertex = end_center_index + 1 + vertex;
            int next_ring_vertex = end_center_index + 1 + vertex + 1;

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

void Line3D::set_width(float p_width) {
    width = p_width;
    generate_mesh();
}

float Line3D::get_width() const {
    return width;
}

void Line3D::set_width_curve(const Ref<Curve> &p_width_curve) {
    // Disconnect from the old width curve
    if (width_curve.is_valid()) {
        Callable callable = Callable(this, "generate_mesh");

        if (width_curve->is_connected("changed", callable)) {
            width_curve->disconnect("changed", callable);
        }
    }

    // Store the new width curve
    width_curve = p_width_curve;

    // Connect to the new width curve
    if (width_curve.is_valid()) {
        Callable callable = Callable(this, "generate_mesh");
        width_curve->connect("changed", callable);
    }
    
    // Rebuild the mesh
    generate_mesh();
}

Ref<Curve> Line3D::get_width_curve() const {
    return width_curve;
}

void Line3D::set_cap_mode(CapMode p_cap_mode) {
    cap_mode = p_cap_mode;
    generate_mesh();
}

Line3D::CapMode Line3D::get_cap_mode() const {
    return cap_mode;
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