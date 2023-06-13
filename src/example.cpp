// example usage of protected_data with shared mutex

#include <optional>
#include <vector>
#include <chrono>
#include <shared_mutex>
#include <thread>
#include <iostream>

#include "protected_data.h"

using namespace std;

// alias for protected_data with shared_mutex
template <typename T>
using shared_protected_data = protected_data<T, std::shared_mutex>;

class Shape
{
protected:
    string name;

public:
    Shape() : name("shape") {};
    Shape(string_view _name) : name(_name) {};

    virtual string_view get_name() const { return name; }
    virtual void set_name(string_view new_name) { name = new_name; }
};

class Square : public Shape
{
    int edge;
    // mark protected_data members as mutable to support safe manipulation in outer shared_guard
    mutable shared_protected_data<vector<int>> other_values;

public:
    Square(int _edge) : Shape("square"), edge(_edge) {};

    int get_edge() const { return edge; }
    void set_edge(int new_edge) { edge = new_edge; }

    // add_value is const and thread-safe since other_values is mutable protected_data
    void add_value(int val) const
    {
        auto guard = other_values.get_unique();
        guard->push_back(val);
    }

    int get_number_of_values() const
    {
        auto guard = other_values.get_shared();
        // guard->clear()  ----- doesn't compile
        return guard->size();
    }


};

class ShapeManager
{
    vector<shared_ptr<shared_protected_data<Shape>>> shapes;

public:
    void add_shape(shared_ptr<shared_protected_data<Shape>> const& pShape)
    {
        shapes.push_back(pShape);
    }

    int get_n_shapes() const
    {
        return shapes.size();
    }

    shared_ptr<shared_protected_data<Shape>> get_shape_at(unsigned int index)
    {
        if (index < shapes.size())
            return shapes[index];
        else
            return nullptr;
    }
};

int main() {

    const Square s(1);
    s.add_value(5);

    ShapeManager manager;

    manager.add_shape(make_shared<shared_protected_data<Shape>>("generic1"));
    {
        auto square_ptr = make_shared<shared_protected_data<Square>>(5);
        manager.add_shape(cast_shared_ptr_protected_data<Shape>(square_ptr).value());
    }
    manager.add_shape(make_shared<shared_protected_data<Shape>>("generic2"));
    manager.add_shape(make_shared<shared_protected_data<Shape>>());

    int N = manager.get_n_shapes();
    for (unsigned int i = 0; i < N; ++i)
    {
        cout << i << endl;
        if (auto pShape = manager.get_shape_at(i))
        {
            // now we have another shared_ptr, to use it we need to lock the guard
            {
                auto s_guard = pShape->get_shared();
                cout << s_guard->get_name() << endl;
                // cannot call s_guard->set_name()
            }

            thread* t = nullptr;
            {
                auto u_guard = pShape->get_unique();

                // below thread will wait until current guard is out of scope
                t = new thread([pShape]() {
                    auto s_guard = pShape->get_shared();
                    cout << "Writing from thread : " << s_guard->get_name() << endl;
                    });

                this_thread::sleep_for(chrono::milliseconds(500));

                u_guard->set_name(string(u_guard->get_name()) + "-1");
            }

            if (t)
            {
                t->join();
                delete t;
            }

            if (auto s_sq_guard = pShape->get_shared_cast<Square>())
            {
                auto& s_guard = s_sq_guard.value();
                // below works because we already protect inner vector
                s_guard->add_value(10);
                // but below doesn't compile
                // s_sq_guard.value()->set_edge(10);
                cout << "Edge of square : " << s_guard->get_edge() << " N values: " << s_guard->get_number_of_values() << endl;
            }
            if (auto u_sq_guard = pShape->get_unique_cast<Square>())
            {
                auto& u_guard = u_sq_guard.value();
                // now also below works
                u_guard->set_edge(10);
                cout << "Edge of square : " << u_guard->get_edge() << endl;
            }
        }
    }

    // call from multiple threads

    auto pShape = manager.get_shape_at(0);

    vector<thread> threads;
    threads.reserve(100);

    for (int i = 0; i < 10; ++i)
    {
        threads.emplace_back([pShape, i]() {
            for (int j = 0; j < 1000; ++j)
            {
                if (j % 2)
                {
                    this_thread::sleep_for(chrono::milliseconds(1));
                    auto s_guard = pShape->get_shared();
                    cout << s_guard->get_name() << endl;
                }
                else
                {
                    auto u_guard = pShape->get_unique();
                    u_guard->set_name("threaded shape-" + to_string(i) + "-" + to_string(j));
                }
            }
            });
    }

    for (auto& t : threads)
        t.join();


    {
        auto s_guard = pShape->get_shared();
        cout << s_guard->get_name() << endl;
    }
}