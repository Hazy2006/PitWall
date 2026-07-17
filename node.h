#pragma once
#include <string>

class Node {
protected:
    std::string name;

public:
    Node(const std::string& node_name) : name(node_name) {}
    virtual ~Node() = default;

    std::string get_name() const { return name; }
    virtual std::string get_type_string() const = 0;
};

class DriverNode : public Node {
public:
    double tire_management_modifier;
    double base_pace_delta;

    DriverNode(const std::string& name, double tire_mod, double pace_delta)
        : Node(name), tire_management_modifier(tire_mod), base_pace_delta(pace_delta) {
    }

    std::string get_type_string() const override { return "Driver"; }
};

class TeamNode : public Node {
public:
    double pit_stop_variance;

    TeamNode(const std::string& name, double pit_variance)
        : Node(name), pit_stop_variance(pit_variance) {
    }

    std::string get_type_string() const override { return "Team"; }
};

class CircuitNode : public Node {
public:
    double base_degradation_rate;

    CircuitNode(const std::string& name, double deg_rate)
        : Node(name), base_degradation_rate(deg_rate) {
    }

    std::string get_type_string() const override { return "Circuit"; }
};