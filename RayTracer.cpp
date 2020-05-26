#include <fstream>
#include <iostream>
#include <cmath>
#include <vector>
#include <chrono>
#include <cinttypes>

#include "vectornd.h"

// https://glm.g-truc.net/
#include <glm\vec3.hpp>
#include <glm\geometric.hpp>
#include <glm\trigonometric.hpp>
#include <glm\gtc\constants.hpp>
#include <glm\gtc\noise.hpp>
//#include <glm\gtc\matrix_transform.hpp>

// https://www.gamedev.net/articles/programming/graphics/ray-tracing-part-1-r3556/

typedef glm::dvec3 Position3;
typedef glm::dvec3 Vector3;
typedef glm::dvec3 Color3;

inline double lenSq(const glm::dvec3& v)
{
    // there is a length2 but it is in the experimental gtx norm
    return dot(v, v);
}

bool solveQuadratic(const double a, const double b, const double c, double &x0, double &x1)
{
    double discr = b * b - 4 * a * c;
    if (discr < 0)
        return false;
    else if (discr == 0)
    {
        x0 = x1 = -0.5 * b / a;
        return true;
    }
    else
    {
        double q = (b > 0) ?
            -0.5 * (b + sqrt(discr)) :
            -0.5 * (b - sqrt(discr));
        x0 = q / a;
        x1 = c / q;
        return true;
    }
}

struct Ray
{
    Position3 origin;
    Vector3 direction;
    Ray(const Position3& origin, const Vector3& direction) : origin(origin), direction(direction) {}
};

struct Sphere
{
    Position3 center;
    double radius;
    Sphere(const Position3& c, double r) : center(c), radius(r) {}

    Vector3 getNormal(const Position3& pi) const { return (pi - center) / radius; }
};

// https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-sphere-intersection
bool intersect(const Ray &ray, const Sphere& s, double &t)
{
    double t0, t1; // solutions for t if the ray intersects
    const double radius_sq = s.radius * s.radius;

    if (false)
    {   // geometric solution
        Vector3 L = s.center - ray.origin;
        double tca = dot(L, ray.direction);
        // if (tca < 0) return false;
        double d2 = dot(L, L) - tca * tca;
        if (d2 > radius_sq)
            return false;
        double thc = sqrt(radius_sq - d2);
        t0 = tca - thc;
        t1 = tca + thc;
    }
    else
    {   // analytic solution
        Vector3 L = ray.origin - s.center;
        double a = lenSq(ray.direction);
        double b = 2 * dot(ray.direction, L);
        double c = lenSq(L) - radius_sq;
        if (!solveQuadratic(a, b, c, t0, t1))
            return false;
    }

    if (t0 > t1)
        std::swap(t0, t1);

    //std::cout << " t0 " << t0 << "\n";
    //std::cout << " t1 " << t1 << "\n";

    if (t0 > 0)
        return t = t0, true;
    else if (t1 > 0)
        return t = t1, true;
    else
        return false;
}

struct Material
{
    Color3 ambient;
    Color3 diffuse;
    Color3 specular;
    double shininess;
};

struct Object
{
    Sphere geom;
    Material mat;
    Object(const Sphere& geom, const Material& mat) : geom(geom), mat(mat) {}
};

struct LightProp
{
    Color3 ambient;
    Color3 diffuse;
    Color3 specular;
};

struct Light
{
    Position3 pos;
    LightProp prop;
    Light(const Position3& pos, const LightProp& prop) : pos(pos), prop(prop) {}
};

struct Scene
{
    Color3 bg;
    Color3 ambience;
    std::vector<Object> objs;
    std::vector<Light> lights;

    Scene(Color3 bg, Color3 ambience) : bg(bg), ambience(ambience) {}

    const Object* intersect(const Ray& ray, double &t) const
    {
        const Object* o = nullptr;
        for (const Object& obj : objs)
        {
            double tt;
            if (::intersect(ray, obj.geom, tt))
            {
                if (tt < t || o == nullptr)
                {
                    //t = tt - glm::epsilon<double>();
                    t = tt - 0.000001;
                    //t = tt;
                    o = &obj;
                }
            }
        }
        return o;
    }

    bool inshadow(const Ray& rayLight, const Vector3& L) const
    {
        // An optimization here is that the intersection loop can exit as soon as any object obsures it, dont necessarily have to find the closest
#if 0
        double tl;
        return (intersect(rayLight, tl) != nullptr && (tl*tl) < lenSq(L));
#else
        double ll = lenSq(L);
        for (const Object& obj : objs)
        {
            double tt;
            if (::intersect(rayLight, obj.geom, tt))
            {
                tt -= 0.000001;
                if ((tt*tt) < ll)
                    return true;
            }
        }
        return false;
#endif
    }

    Color3 lighting(const Position3 incidence, const Vector3 normal, const Vector3 rayDirection, const Material& mat) const
    {

        Color3 ambient(0, 0, 0);
        Color3 diffuse(0, 0, 0);
        Color3 specular(0, 0, 0);

        // if (false)
        {
            ambient += ambience * mat.ambient;
        }

        for (const Light& l : lights)
        {
            const Vector3 L = l.pos - incidence;
            const Ray rayLight(incidence, normalize(L));
            if (!inshadow(rayLight, L))
            {
                ambient += l.prop.ambient * mat.ambient;

                // TOOD: diffuse and specular.
                // See https://github.com/g-truc/glm/blob/master/manual.md#-84-lighting
                // and https://learnopengl.com/Lighting/Materials

                const double diff = dot(rayLight.direction, normal);
                if (diff > 0)
                {
                    // if (isDiffuse)
                    // if (false)
                    {
                        //Color3 light_col = l.color * diffuse;
                        //Color3 light_col = l.color * glm::one_over_pi<double>() * diffuse * (1 / lenSq(L));
                        Color3 light_col = l.prop.diffuse * diff * mat.diffuse;
                        diffuse += light_col;
                    }

                    // if (isSpecular)
                    // if (false)
                    {
                        const Vector3 reflect = glm::reflect(-rayLight.direction, normal);
                        const Vector3 view = -rayDirection;
                        const double specularA = dot(reflect, view);
                        if (specularA > 0 && mat.shininess > 0)
                        {
                            const double specularB = pow(specularA, mat.shininess);
                            specular += l.prop.specular * specularB * mat.specular;
                        }
                    }
                }
            }
        }

        return clamp(ambient + diffuse + specular, 0.0, 1.0);
    }

    Color3 cast(const Ray& ray) const
    {
        double t;
        const Object* o = intersect(ray, t);
        if (o != nullptr)
        {
            //return o->color;

            const Position3 incidence = ray.origin + ray.direction * t;
            const Vector3 N = normalize(o->geom.getNormal(incidence));
            // TODO Test incidence again with reflected ray for mirror objects

            return lighting(incidence, N, ray.direction, o->mat);
        }
        else
            return bg;
    }
};

class CameraOrtho
{
public:
    Ray getRay(const double u, const double v) const
    {
        return Ray(Vector3(u, v, 0), normalize(Vector3(0, 0, 1)));
    }
};

class CameraPerspective
{
public:
    CameraPerspective(const double fov)
        : fovFactor(1 / tan(fov / 2))
    {
    }

    Ray getRay(const double u, const double v) const
    {
        return Ray(Position3(0, 0, 0), normalize(Vector3(u, v, fovFactor)));
    }

private:
    double fovFactor;
};

void SavePPM(const char* filename, const vectornd<Color3, 2>& data)
{
    const vectornd<Color3, 2>::size_type s = data.size();

    const int colors = 255;

    std::ofstream out(filename);
    //std::ostream& out(std::cout);
    out << "P3\n" << s[1] << ' ' << s[0] << ' ' << colors << '\n';

    vectornd<Color3, 2>::size_type i;
    for (i[0] = 0; i[0] < s[0]; ++i[0])
    {
        for (i[1] = 0; i[1] < s[1]; ++i[1])
        {
            const Color3 pix_col = data[i] * Color3::value_type(colors);

            out << std::lrint(pix_col.r) << ' '
                << std::lrint(pix_col.g) << ' '
                << std::lrint(pix_col.b) << '\n';
        }
    }
}

int main()
{
    const Color3 black(0, 0, 0);
    const Color3 white(1, 1, 1);
    const Color3 red(1, 0, 0);
    const Color3 green(0, 1, 0);
    const Color3 blue(0, 0, 1);

    // http://www.barradeau.com/nicoptere/dump/materials.html
    // http://devernay.free.fr/cours/opengl/materials.html
    const Material mattest = { red * 0.2, red * 0.4, white * 0.9, 30 };
    const Material mattest2 = { Color3(1.0, 0.5, 0.31), Color3(1.0, 0.5, 0.31), Color3(0.5, 0.5, 0.5), 32 };
    const Material brass = { Color3(0.329412, 0.223529, 0.027451), Color3(0.780392, 0.568627, 0.113725), Color3(0.992157, 0.941176, 0.807843), 27.8974 };
    const Material jade = { Color3(0.135, 0.2225, 0.1575), Color3(0.54, 0.89, 0.63), Color3(0.316228, 0.316228, 0.316228), 12.8 };

    //const Color3 ltest(white);
    const LightProp ltest = { white * 0.2, white * 0.5, white };

    Scene s(white * 0.1, white * 0.15);
    const char* filename = nullptr;

    switch (2)
    {
    case 1:
        filename = "out1.ppm";
        s.objs.push_back(Object(Sphere(Vector3(-0.3, 0, 3), 0.7), mattest2));
        s.lights.push_back(Light(Vector3(1.5, 0, 1.5), ltest));
        break;

    case 2:
        filename = "out2.ppm";
        s.objs.push_back(Object(Sphere(Vector3(-0.3, 0, 1.5), 0.7), brass));
        s.objs.push_back(Object(Sphere(Vector3(0.5, 0, 0.7), 0.2), jade));
        s.lights.push_back(Light(Vector3(1.5, 0, 0), ltest));
        break;

    case 3:
        filename = "out3.ppm";
        s.objs.push_back(Object(Sphere(Vector3(0.3, 0, 3), 0.2), mattest));
        s.objs.push_back(Object(Sphere(Vector3(-0.3, 0, 1), 0.2), mattest));
        s.lights.push_back(Light(Vector3(0, 1, 1), ltest));
        break;
    }

    //CameraOrtho camera;
    CameraPerspective camera(glm::radians(90.0));

    vectornd<Color3, 2> data({ 480, 640 });
    const vectornd<Color3, 2>::size_type sz = data.size();

    const double aspect = (double) sz[1] / sz[0];

    {
        const auto start = std::chrono::high_resolution_clock::now();
        vectornd<Color3, 2>::size_type i;
        for (i[0] = 0; i[0] < sz[0]; ++i[0])
        {
            for (i[1] = 0; i[1] < sz[1]; ++i[1])
            {
                const double u = (2 * i[1] / (double) sz[1] - 1) * aspect;
                const double v = (2 * i[0] / (double) sz[0] - 1);

                const Ray ray = camera.getRay(u, v);

                data[i] = s.cast(ray);
            }

            const double l = (double) i[0] / (sz[0] - 1);
            fprintf(stderr, "\rRendering [%.*s%.*s] %6.2f%%", std::lrint(l * 20), "....................", 20 - std::lrint(l * 20), "                    ", l * 100);
        }

        const auto stop = std::chrono::high_resolution_clock::now();
        fprintf(stderr, " %" PRId64  " msec\n", std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count());
    }

    {
        const auto start = std::chrono::high_resolution_clock::now();
        fprintf(stderr, "Saving...");
        SavePPM(filename, data);
        const auto stop = std::chrono::high_resolution_clock::now();
        fprintf(stderr, " %" PRId64  " msec\n", std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count());
    }
}
