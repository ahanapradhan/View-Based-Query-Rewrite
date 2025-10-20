#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <sstream>
#include <algorithm>
#include <memory>
#include <queue>

// Edge types in the graph
enum class EdgeType {
    JOIN,           // Solid: equality join
    RELATIONAL,     // Dashed: same relation
    GROUPBY         // Dotted: group-by aggregate
};

// Edge structure with weight
struct Edge {
    std::string from;
    std::string to;
    EdgeType type;
    int weight;
    
    Edge(const std::string& f, const std::string& t, EdgeType et, int w = 1)
        : from(f), to(t), type(et), weight(w) {}
};

// Annotation for nodes (constraints, predicates)
struct Annotation {
    std::string constraint;
    bool is_compulsory;
    
    Annotation(const std::string& c = "", bool comp = false)
        : constraint(c), is_compulsory(comp) {}
    
    bool intersects(const Annotation& other) const {
        if (constraint.empty() || other.constraint.empty()) return true;
        return constraint == other.constraint;
    }
};

// Graph node representing an attribute
struct Node {
    std::string name;
    std::string relation;
    std::vector<Annotation> annotations;
    
    Node(const std::string& n, const std::string& r = "")
        : name(n), relation(r) {}
    
    void addAnnotation(const Annotation& ann) {
        annotations.push_back(ann);
    }
    
    bool hasCompatibleAnnotation(const Node& other) const {
        if (annotations.empty() || other.annotations.empty()) return true;
        for (const auto& a1 : annotations) {
            for (const auto& a2 : other.annotations) {
                if (a1.intersects(a2)) return true;
            }
        }
        return false;
    }
};

// Graph structure
class Graph {
public:
    std::map<std::string, Node> nodes;
    std::vector<Edge> edges;
    
    void addNode(const Node& node) {
        nodes[node.name] = node;
    }
    
    void addEdge(const Edge& edge) {
        edges.push_back(edge);
    }
    
    bool hasNode(const std::string& name) const {
        return nodes.find(name) != nodes.end();
    }
    
    // Check if graph is connected for given projection attributes
    bool isConnected(const std::vector<std::string>& projections) const {
        if (projections.empty()) return false;
        
        // Check all projections exist
        for (const auto& proj : projections) {
            if (!hasNode(proj)) return false;
        }
        
        if (projections.size() == 1) return true;
        
        // Build adjacency list
        std::map<std::string, std::set<std::string>> adj;
        for (const auto& edge : edges) {
            adj[edge.from].insert(edge.to);
            adj[edge.to].insert(edge.from);
        }
        
        // BFS from first projection
        std::set<std::string> visited;
        std::queue<std::string> q;
        q.push(projections[0]);
        visited.insert(projections[0]);
        
        while (!q.empty()) {
            std::string curr = q.front();
            q.pop();
            
            for (const auto& neighbor : adj[curr]) {
                if (visited.find(neighbor) == visited.end()) {
                    visited.insert(neighbor);
                    q.push(neighbor);
                }
            }
        }
        
        // Check if all projections are reachable
        for (const auto& proj : projections) {
            if (visited.find(proj) == visited.end()) {
                return false;
            }
        }
        
        return true;
    }
    
    void print() const {
        std::cout << "Nodes:\n";
        for (const auto& [name, node] : nodes) {
            std::cout << "  " << name << " (relation: " << node.relation << ")\n";
            for (const auto& ann : node.annotations) {
                std::cout << "    - " << ann.constraint 
                         << (ann.is_compulsory ? " [compulsory]" : " [optional]") << "\n";
            }
        }
        
        std::cout << "Edges:\n";
        for (const auto& edge : edges) {
            std::cout << "  " << edge.from << " -> " << edge.to;
            std::cout << " [";
            switch (edge.type) {
                case EdgeType::JOIN: std::cout << "JOIN"; break;
                case EdgeType::RELATIONAL: std::cout << "RELATIONAL"; break;
                case EdgeType::GROUPBY: std::cout << "GROUPBY"; break;
            }
            std::cout << ", weight=" << edge.weight << "]\n";
        }
    }
};

// Simple SQL parser for SELECT queries
class SQLParser {
public:
    struct ParsedQuery {
        std::vector<std::string> projections;
        std::vector<std::string> tables;
        std::map<std::string, std::string> joins; // attr -> attr
        std::map<std::string, std::string> attr_to_table;
    };
    
    ParsedQuery parse(const std::string& query) {
        ParsedQuery pq;
        std::string q = toLower(query);
        
        // Extract SELECT clause
        size_t select_pos = q.find("select");
        size_t from_pos = q.find("from");
        if (select_pos == std::string::npos || from_pos == std::string::npos) {
            return pq;
        }
        
        std::string select_clause = q.substr(select_pos + 6, from_pos - select_pos - 6);
        pq.projections = split(trim(select_clause), ',');
        
        // Extract FROM clause
        size_t where_pos = q.find("where");
        std::string from_clause;
        if (where_pos != std::string::npos) {
            from_clause = q.substr(from_pos + 4, where_pos - from_pos - 4);
        } else {
            from_clause = q.substr(from_pos + 4);
        }
        pq.tables = split(trim(from_clause), ',');
        
        // Extract WHERE clause (joins)
        if (where_pos != std::string::npos) {
            std::string where_clause = q.substr(where_pos + 5);
            auto predicates = split(where_clause, "and");
            
            for (const auto& pred : predicates) {
                size_t eq_pos = pred.find('=');
                if (eq_pos != std::string::npos) {
                    std::string left = trim(pred.substr(0, eq_pos));
                    std::string right = trim(pred.substr(eq_pos + 1));
                    pq.joins[left] = right;
                }
            }
        }
        
        return pq;
    }
    
private:
    std::string toLower(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        return s;
    }
    
    std::string trim(const std::string& s) {
        size_t start = s.find_first_not_of(" \t\n\r");
        size_t end = s.find_last_not_of(" \t\n\r");
        if (start == std::string::npos) return "";
        return s.substr(start, end - start + 1);
    }
    
    std::vector<std::string> split(const std::string& s, char delim) {
        std::vector<std::string> result;
        std::stringstream ss(s);
        std::string item;
        while (std::getline(ss, item, delim)) {
            result.push_back(trim(item));
        }
        return result;
    }
    
    std::vector<std::string> split(const std::string& s, const std::string& delim) {
        std::vector<std::string> result;
        size_t start = 0;
        size_t end = s.find(delim);
        
        while (end != std::string::npos) {
            result.push_back(trim(s.substr(start, end - start)));
            start = end + delim.length();
            end = s.find(delim, start);
        }
        result.push_back(trim(s.substr(start)));
        return result;
    }
};

// Compliance rule
struct ComplianceRule {
    std::string location;
    std::string attribute;
    std::string relation;
    bool can_transfer;
    std::string constraint;
    
    ComplianceRule(const std::string& loc, const std::string& attr,
                   const std::string& rel, bool transfer, const std::string& cons = "")
        : location(loc), attribute(attr), relation(rel), 
          can_transfer(transfer), constraint(cons) {}
};

// Main compliance checker
class ComplianceChecker {
private:
    Graph query_graph;
    std::map<std::string, Graph> compliance_forests;
    std::vector<ComplianceRule> rules;
    std::string result_location;
    SQLParser parser;
    
public:
    void setResultLocation(const std::string& loc) {
        result_location = loc;
    }
    
    void addComplianceRule(const ComplianceRule& rule) {
        rules.push_back(rule);
    }
    
    // Build query graph from SQL
    void buildQueryGraph(const std::string& query) {
        auto pq = parser.parse(query);
        
        // Add projection nodes
        for (const auto& proj : pq.projections) {
            Node n(proj);
            query_graph.addNode(n);
        }
        
        // Add join edges
        for (const auto& [left, right] : pq.joins) {
            // Determine if attributes are from same relation
            bool same_relation = false;
            std::string left_table, right_table;
            
            // Simple heuristic: attribute format is table.attr or attr
            size_t dot_pos = left.find('.');
            if (dot_pos != std::string::npos) {
                left_table = left.substr(0, dot_pos);
            }
            
            dot_pos = right.find('.');
            if (dot_pos != std::string::npos) {
                right_table = right.substr(0, dot_pos);
            }
            
            same_relation = !left_table.empty() && left_table == right_table;
            
            // Add nodes if not projections
            if (!query_graph.hasNode(left)) {
                Node n(left, left_table);
                query_graph.addNode(n);
            }
            if (!query_graph.hasNode(right)) {
                Node n(right, right_table);
                query_graph.addNode(n);
            }
            
            EdgeType et = same_relation ? EdgeType::RELATIONAL : EdgeType::JOIN;
            query_graph.addEdge(Edge(left, right, et, 1));
        }
    }
    
    // Build compliance forest for each location
    void buildComplianceForests() {
        for (const auto& rule : rules) {
            if (compliance_forests.find(rule.location) == compliance_forests.end()) {
                compliance_forests[rule.location] = Graph();
            }
            
            Graph& cf = compliance_forests[rule.location];
            
            // Add node with annotation
            Node n(rule.attribute, rule.relation);
            n.addAnnotation(Annotation(rule.constraint, true));
            cf.addNode(n);
        }
    }
    
    // Compute view at location Li intersected with LR
    Graph computeView(const std::string& location) {
        Graph view;
        
        if (compliance_forests.find(location) == compliance_forests.end()) {
            return view;
        }
        
        Graph& cf_li = compliance_forests[location];
        Graph& cf_lr = compliance_forests[result_location];
        
        // Intersect nodes: QG ∩ CF_Li ∩ CF_LR
        for (const auto& [name, qnode] : query_graph.nodes) {
            if (cf_li.hasNode(name)) {
                Node& li_node = cf_li.nodes[name];
                
                // Check if can transfer to LR
                bool can_go_to_lr = true;
                if (cf_lr.nodes.size() > 0 && !cf_lr.hasNode(name)) {
                    // Check if there's a rule blocking transfer to LR
                    bool found_blocking_rule = false;
                    for (const auto& rule : rules) {
                        if (rule.location == result_location && 
                            rule.attribute == name && !rule.can_transfer) {
                            found_blocking_rule = true;
                            break;
                        }
                    }
                    if (found_blocking_rule) can_go_to_lr = false;
                }
                
                if (can_go_to_lr && qnode.hasCompatibleAnnotation(li_node)) {
                    view.addNode(qnode);
                }
            }
        }
        
        // Add edges with weight < 3
        for (const auto& edge : query_graph.edges) {
            if (edge.weight < 3 && view.hasNode(edge.from) && view.hasNode(edge.to)) {
                view.addEdge(edge);
            }
        }
        
        return view;
    }
    
    // Merge all views
    Graph mergeViews(const std::vector<Graph>& views) {
        Graph merged;
        
        for (const auto& view : views) {
            for (const auto& [name, node] : view.nodes) {
                if (!merged.hasNode(name)) {
                    merged.addNode(node);
                }
            }
            
            for (const auto& edge : view.edges) {
                merged.addEdge(edge);
            }
        }
        
        return merged;
    }
    
    // Main compliance check
    bool isCompliant(const std::string& query) {
        // Build query graph
        buildQueryGraph(query);
        
        // Build compliance forests
        buildComplianceForests();
        
        // Get projections
        auto pq = parser.parse(query);
        
        // Compute views for each location
        std::vector<Graph> views;
        for (const auto& [loc, cf] : compliance_forests) {
            if (loc != result_location) {
                Graph view = computeView(loc);
                if (view.nodes.size() > 0) {
                    views.push_back(view);
                }
            }
        }
        
        // Merge all views
        Graph merged = mergeViews(views);
        
        // Check connectivity
        return merged.isConnected(pq.projections);
    }
    
    void printDebugInfo() {
        std::cout << "\n=== Query Graph ===\n";
        query_graph.print();
        
        std::cout << "\n=== Compliance Forests ===\n";
        for (const auto& [loc, cf] : compliance_forests) {
            std::cout << "\nLocation: " << loc << "\n";
            cf.print();
        }
    }
};

// Example usage
int main() {
    ComplianceChecker checker;
    
    // Set result location
    checker.setResultLocation("LR");
    
    // Define compliance rules
    // L1: customer, orders
    checker.addComplianceRule(ComplianceRule("L1", "c_name", "customer", true));
    checker.addComplianceRule(ComplianceRule("L1", "c_nationkey", "customer", true));
    checker.addComplianceRule(ComplianceRule("L1", "o_orderkey", "orders", true));
    
    // L2: nation, region
    checker.addComplianceRule(ComplianceRule("L2", "n_name", "nation", true));
    checker.addComplianceRule(ComplianceRule("L2", "n_nationkey", "nation", true));
    checker.addComplianceRule(ComplianceRule("L2", "r_name", "region", true));
    
    // L3: supplier, partsupp, part
    checker.addComplianceRule(ComplianceRule("L3", "s_name", "supplier", true));
    checker.addComplianceRule(ComplianceRule("L3", "s_nationkey", "supplier", true));
    
    // LR: output projections can be received
    checker.addComplianceRule(ComplianceRule("LR", "c_name", "", true));
    checker.addComplianceRule(ComplianceRule("LR", "n_name", "", true));
    checker.addComplianceRule(ComplianceRule("LR", "s_name", "", true));
    
    // Example query from the paper
    std::string query = "SELECT c_name, n_name, s_name FROM customer, nation, supplier "
                       "WHERE c_nationkey = n_nationkey AND n_nationkey = s_nationkey";
    
    std::cout << "Query: " << query << "\n";
    std::cout << "\n=== Checking Compliance ===\n";
    
    bool compliant = checker.isCompliant(query);
    
    checker.printDebugInfo();
    
    std::cout << "\n=== Result ===\n";
    std::cout << "Query is " << (compliant ? "COMPLIANT" : "NON-COMPLIANT") 
              << " at location LR\n";
    
    // Test non-compliant query
    std::cout << "\n\n=== Testing Non-Compliant Query ===\n";
    ComplianceChecker checker2;
    checker2.setResultLocation("LR");
    
    // Restricted rule: c_name cannot leave L1
    checker2.addComplianceRule(ComplianceRule("L1", "c_name", "customer", false));
    checker2.addComplianceRule(ComplianceRule("L1", "c_nationkey", "customer", true));
    checker2.addComplianceRule(ComplianceRule("L2", "n_name", "nation", true));
    checker2.addComplianceRule(ComplianceRule("L2", "n_nationkey", "nation", true));
    
    std::string query2 = "SELECT c_name, n_name FROM customer, nation "
                        "WHERE c_nationkey = n_nationkey";
    
    std::cout << "Query: " << query2 << "\n";
    bool compliant2 = checker2.isCompliant(query2);
    
    std::cout << "\nQuery is " << (compliant2 ? "COMPLIANT" : "NON-COMPLIANT") 
              << " at location LR\n";
    
    return 0;
}
