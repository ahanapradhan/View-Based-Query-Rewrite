#include <iostream>
#include <vector>
#include <string>
#include <fstream>

/*
 * TPC-H Schema Reference:
 * 
 * CUSTOMER (c_custkey, c_name, c_address, c_nationkey, c_phone, c_acctbal, c_mktsegment, c_comment)
 * ORDERS (o_orderkey, o_custkey, o_orderstatus, o_totalprice, o_orderdate, o_orderpriority, o_clerk, o_shippriority, o_comment)
 * LINEITEM (l_orderkey, l_partkey, l_suppkey, l_linenumber, l_quantity, l_extendedprice, l_discount, l_tax, l_returnflag, l_linestatus, l_shipdate, l_commitdate, l_receiptdate, l_shipinstruct, l_shipmode, l_comment)
 * PART (p_partkey, p_name, p_mfgr, p_brand, p_type, p_size, p_container, p_retailprice, p_comment)
 * SUPPLIER (s_suppkey, s_name, s_address, s_nationkey, s_phone, s_acctbal, s_comment)
 * PARTSUPP (ps_partkey, ps_suppkey, ps_availqty, ps_supplycost, ps_comment)
 * NATION (n_nationkey, n_name, n_regionkey, n_comment)
 * REGION (r_regionkey, r_name, r_comment)
 */

struct TestCase {
    int id;
    std::string description;
    std::string query;
    std::vector<std::string> views;
    bool should_have_rewriting;
};

std::vector<TestCase> generateTestCases() {
    std::vector<TestCase> testcases;
    
    // Test Case 1: Simple Customer-Orders join
    testcases.push_back({1, "Simple two-table join", 
        "SELECT c.c_name, o.o_orderkey FROM Customer c, Orders o WHERE c.c_custkey = o.o_custkey",
        {
            "SELECT c.c_custkey, c.c_name FROM Customer c",
            "SELECT o.o_orderkey, o.o_custkey FROM Orders o",
            "SELECT c.c_name, c.c_address FROM Customer c",
            "SELECT o.o_orderkey, o.o_totalprice FROM Orders o",
            "SELECT c.c_custkey, c.c_nationkey FROM Customer c"
        },
        true
    });
    
    // Test Case 2: Three-table join
    testcases.push_back({2, "Three-table join with LineItem",
        "SELECT o.o_orderkey, c.c_name, l.l_quantity FROM Orders o, Customer c, LineItem l WHERE o.o_custkey = c.c_custkey AND o.o_orderkey = l.l_orderkey",
        {
            "SELECT o.o_orderkey, o.o_custkey FROM Orders o",
            "SELECT c.c_custkey, c.c_name FROM Customer c",
            "SELECT l.l_orderkey, l.l_quantity FROM LineItem l",
            "SELECT o.o_orderkey, o.o_totalprice FROM Orders o",
            "SELECT c.c_name, c.c_nationkey FROM Customer c",
            "SELECT l.l_quantity, l.l_extendedprice FROM LineItem l"
        },
        true
    });
    
    // Test Case 3: Part-Supplier join
    testcases.push_back({3, "Part-Supplier through PartSupp",
        "SELECT p.p_name, s.s_name FROM Part p, PartSupp ps, Supplier s WHERE p.p_partkey = ps.ps_partkey AND ps.ps_suppkey = s.s_suppkey",
        {
            "SELECT p.p_partkey, p.p_name FROM Part p",
            "SELECT ps.ps_partkey, ps.ps_suppkey FROM PartSupp ps",
            "SELECT s.s_suppkey, s.s_name FROM Supplier s",
            "SELECT p.p_name, p.p_type FROM Part p",
            "SELECT s.s_name, s.s_address FROM Supplier s",
            "SELECT ps.ps_availqty, ps.ps_supplycost FROM PartSupp ps"
        },
        true
    });
    
    // Test Case 4: Nation-Region join
    testcases.push_back({4, "Simple Nation-Region join",
        "SELECT n.n_name, r.r_name FROM Nation n, Region r WHERE n.n_regionkey = r.r_regionkey",
        {
            "SELECT n.n_nationkey, n.n_name, n.n_regionkey FROM Nation n",
            "SELECT r.r_regionkey, r.r_name FROM Region r",
            "SELECT n.n_name, n.n_comment FROM Nation n",
            "SELECT r.r_name, r.r_comment FROM Region r",
            "SELECT n.n_nationkey, n.n_regionkey FROM Nation n"
        },
        true
    });
    
    // Test Case 5: Customer with Nation
    testcases.push_back({5, "Customer-Nation join",
        "SELECT c.c_name, n.n_name FROM Customer c, Nation n WHERE c.c_nationkey = n.n_nationkey",
        {
            "SELECT c.c_custkey, c.c_name, c.c_nationkey FROM Customer c",
            "SELECT n.n_nationkey, n.n_name FROM Nation n",
            "SELECT c.c_name, c.c_address FROM Customer c",
            "SELECT n.n_name, n.n_regionkey FROM Nation n",
            "SELECT c.c_nationkey, c.c_mktsegment FROM Customer c"
        },
        true
    });
    
    // Test Case 6: Supplier with Nation
    testcases.push_back({6, "Supplier-Nation join",
        "SELECT s.s_name, n.n_name FROM Supplier s, Nation n WHERE s.s_nationkey = n.n_nationkey",
        {
            "SELECT s.s_suppkey, s.s_name, s.s_nationkey FROM Supplier s",
            "SELECT n.n_nationkey, n.n_name FROM Nation n",
            "SELECT s.s_name, s.s_address FROM Supplier s",
            "SELECT n.n_name, n.n_regionkey FROM Nation n",
            "SELECT s.s_nationkey, s.s_phone FROM Supplier s",
            "SELECT n.n_nationkey, n.n_comment FROM Nation n"
        },
        true
    });
    
    // Test Case 7: Four-table join
    testcases.push_back({7, "Customer-Orders-LineItem-Part chain",
        "SELECT c.c_name, o.o_orderkey, l.l_quantity, p.p_name FROM Customer c, Orders o, LineItem l, Part p WHERE c.c_custkey = o.o_custkey AND o.o_orderkey = l.l_orderkey AND l.l_partkey = p.p_partkey",
        {
            "SELECT c.c_custkey, c.c_name FROM Customer c",
            "SELECT o.o_orderkey, o.o_custkey FROM Orders o",
            "SELECT l.l_orderkey, l.l_partkey, l.l_quantity FROM LineItem l",
            "SELECT p.p_partkey, p.p_name FROM Part p",
            "SELECT c.c_name, c.c_nationkey FROM Customer c",
            "SELECT o.o_orderkey, o.o_totalprice FROM Orders o",
            "SELECT l.l_quantity, l.l_extendedprice FROM LineItem l",
            "SELECT p.p_name, p.p_type FROM Part p"
        },
        true
    });
    
    // Test Case 8: Pre-joined view (should use single view)
    testcases.push_back({8, "Pre-joined Customer-Orders view available",
        "SELECT c.c_name, o.o_orderkey FROM Customer c, Orders o WHERE c.c_custkey = o.o_custkey",
        {
            "SELECT c.c_name, o.o_orderkey FROM Customer c, Orders o WHERE c.c_custkey = o.o_custkey",
            "SELECT c.c_custkey, c.c_name FROM Customer c",
            "SELECT o.o_orderkey, o.o_custkey FROM Orders o",
            "SELECT c.c_name, c.c_address FROM Customer c",
            "SELECT o.o_totalprice, o.o_orderdate FROM Orders o"
        },
        true
    });
    
    // Test Case 9: Incomplete views (missing join attribute)
    testcases.push_back({9, "Views missing critical join attribute",
        "SELECT c.c_name, o.o_orderkey FROM Customer c, Orders o WHERE c.c_custkey = o.o_custkey",
        {
            "SELECT c.c_name, c.c_address FROM Customer c",
            "SELECT o.o_orderkey, o.o_totalprice FROM Orders o",
            "SELECT c.c_mktsegment, c.c_phone FROM Customer c",
            "SELECT o.o_orderdate, o.o_orderpriority FROM Orders o",
            "SELECT c.c_name, c.c_comment FROM Customer c"
        },
        false
    });
    
    // Test Case 10: LineItem with Part and Supplier
    testcases.push_back({10, "LineItem-Part-Supplier join",
        "SELECT l.l_quantity, p.p_name, s.s_name FROM LineItem l, Part p, Supplier s WHERE l.l_partkey = p.p_partkey AND l.l_suppkey = s.s_suppkey",
        {
            "SELECT l.l_orderkey, l.l_partkey, l.l_suppkey, l.l_quantity FROM LineItem l",
            "SELECT p.p_partkey, p.p_name FROM Part p",
            "SELECT s.s_suppkey, s.s_name FROM Supplier s",
            "SELECT l.l_quantity, l.l_extendedprice FROM LineItem l",
            "SELECT p.p_name, p.p_type FROM Part p",
            "SELECT s.s_name, s.s_nationkey FROM Supplier s"
        },
        true
    });
    
    // Test Case 11: Single table query
    testcases.push_back({11, "Single table projection",
        "SELECT c.c_name, c.c_address FROM Customer c",
        {
            "SELECT c.c_custkey, c.c_name, c.c_address FROM Customer c",
            "SELECT c.c_name, c.c_address, c.c_phone FROM Customer c",
            "SELECT c.c_custkey, c.c_nationkey FROM Customer c",
            "SELECT c.c_address, c.c_mktsegment FROM Customer c",
            "SELECT c.c_name, c.c_acctbal FROM Customer c"
        },
        true
    });
    
    // Test Case 12: Five-table join
    testcases.push_back({12, "Complex five-table join",
        "SELECT c.c_name, n.n_name, r.r_name, o.o_orderkey FROM Customer c, Nation n, Region r, Orders o WHERE c.c_nationkey = n.n_nationkey AND n.n_regionkey = r.r_regionkey AND c.c_custkey = o.o_custkey",
        {
            "SELECT c.c_custkey, c.c_name, c.c_nationkey FROM Customer c",
            "SELECT n.n_nationkey, n.n_name, n.n_regionkey FROM Nation n",
            "SELECT r.r_regionkey, r.r_name FROM Region r",
            "SELECT o.o_orderkey, o.o_custkey FROM Orders o",
            "SELECT c.c_name, c.c_address FROM Customer c",
            "SELECT n.n_name, n.n_comment FROM Nation n",
            "SELECT r.r_name, r.r_comment FROM Region r",
            "SELECT o.o_orderkey, o.o_totalprice FROM Orders o"
        },
        true
    });
    
    // Test Case 13: Orders with multiple attributes
    testcases.push_back({13, "Orders projection with multiple attributes",
        "SELECT o.o_orderkey, o.o_totalprice, o.o_orderdate FROM Orders o",
        {
            "SELECT o.o_orderkey, o.o_custkey, o.o_totalprice FROM Orders o",
            "SELECT o.o_orderkey, o.o_orderdate, o.o_orderpriority FROM Orders o",
            "SELECT o.o_totalprice, o.o_orderdate FROM Orders o",
            "SELECT o.o_orderkey, o.o_orderstatus FROM Orders o",
            "SELECT o.o_orderdate, o.o_clerk FROM Orders o",
            "SELECT o.o_orderkey, o.o_totalprice, o.o_orderdate, o.o_orderpriority FROM Orders o"
        },
        true
    });
    
    // Test Case 14: Supplier-PartSupp-Part complete chain
    testcases.push_back({14, "Supplier to Part through PartSupp",
        "SELECT s.s_name, ps.ps_availqty, p.p_name FROM Supplier s, PartSupp ps, Part p WHERE s.s_suppkey = ps.ps_suppkey AND ps.ps_partkey = p.p_partkey",
        {
            "SELECT s.s_suppkey, s.s_name FROM Supplier s",
            "SELECT ps.ps_partkey, ps.ps_suppkey, ps.ps_availqty FROM PartSupp ps",
            "SELECT p.p_partkey, p.p_name FROM Part p",
            "SELECT s.s_name, s.s_nationkey FROM Supplier s",
            "SELECT ps.ps_availqty, ps.ps_supplycost FROM PartSupp ps",
            "SELECT p.p_name, p.p_type FROM Part p",
            "SELECT s.s_suppkey, s.s_address FROM Supplier s"
        },
        true
    });
    
    // Test Case 15: Customer-Nation-Region chain
    testcases.push_back({15, "Customer through Nation to Region",
        "SELECT c.c_name, n.n_name, r.r_name FROM Customer c, Nation n, Region r WHERE c.c_nationkey = n.n_nationkey AND n.n_regionkey = r.r_regionkey",
        {
            "SELECT c.c_custkey, c.c_name, c.c_nationkey FROM Customer c",
            "SELECT n.n_nationkey, n.n_name, n.n_regionkey FROM Nation n",
            "SELECT r.r_regionkey, r.r_name FROM Region r",
            "SELECT c.c_name, c.c_mktsegment FROM Customer c",
            "SELECT n.n_name, n.n_comment FROM Nation n",
            "SELECT r.r_name, r.r_comment FROM Region r"
        },
        true
    });
    
    // Test Case 16: Two separate joins (star schema)
    testcases.push_back({16, "Orders with Customer and LineItem separately",
        "SELECT o.o_orderkey, c.c_name, l.l_quantity FROM Orders o, Customer c, LineItem l WHERE o.o_custkey = c.c_custkey AND o.o_orderkey = l.l_orderkey",
        {
            "SELECT o.o_orderkey, o.o_custkey FROM Orders o",
            "SELECT c.c_custkey, c.c_name FROM Customer c",
            "SELECT l.l_orderkey, l.l_quantity FROM LineItem l",
            "SELECT o.o_totalprice, o.o_orderdate FROM Orders o",
            "SELECT c.c_name, c.c_nationkey FROM Customer c",
            "SELECT l.l_quantity, l.l_discount FROM LineItem l",
            "SELECT o.o_orderkey, o.o_orderstatus FROM Orders o"
        },
        true
    });
    
    // Test Case 17: Part details only
    testcases.push_back({17, "Part table projection",
        "SELECT p.p_name, p.p_type, p.p_size FROM Part p",
        {
            "SELECT p.p_partkey, p.p_name FROM Part p",
            "SELECT p.p_name, p.p_type, p.p_size FROM Part p",
            "SELECT p.p_type, p.p_size, p.p_container FROM Part p",
            "SELECT p.p_name, p.p_retailprice FROM Part p",
            "SELECT p.p_partkey, p.p_mfgr FROM Part p",
            "SELECT p.p_size, p.p_comment FROM Part p"
        },
        true
    });
    
    // Test Case 18: Missing intermediate table
    testcases.push_back({18, "Customer to Region without Nation view",
        "SELECT c.c_name, n.n_name, r.r_name FROM Customer c, Nation n, Region r WHERE c.c_nationkey = n.n_nationkey AND n.n_regionkey = r.r_regionkey",
        {
            "SELECT c.c_custkey, c.c_name, c.c_nationkey FROM Customer c",
            "SELECT r.r_regionkey, r.r_name FROM Region r",
            "SELECT c.c_name, c.c_address FROM Customer c",
            "SELECT r.r_name, r.r_comment FROM Region r",
            "SELECT c.c_nationkey, c.c_phone FROM Customer c"
        },
        false
    });
    
    // Test Case 19: LineItem-Orders simple join
    testcases.push_back({19, "LineItem-Orders join",
        "SELECT l.l_orderkey, l.l_quantity, o.o_totalprice FROM LineItem l, Orders o WHERE l.l_orderkey = o.o_orderkey",
        {
            "SELECT l.l_orderkey, l.l_partkey, l.l_quantity FROM LineItem l",
            "SELECT o.o_orderkey, o.o_totalprice FROM Orders o",
            "SELECT l.l_quantity, l.l_extendedprice FROM LineItem l",
            "SELECT o.o_orderkey, o.o_custkey FROM Orders o",
            "SELECT l.l_orderkey, l.l_suppkey FROM LineItem l",
            "SELECT o.o_totalprice, o.o_orderdate FROM Orders o"
        },
        true
    });
    
    // Test Case 20: PartSupp details
    testcases.push_back({20, "PartSupp projection",
        "SELECT ps.ps_partkey, ps.ps_suppkey, ps.ps_availqty FROM PartSupp ps",
        {
            "SELECT ps.ps_partkey, ps.ps_suppkey FROM PartSupp ps",
            "SELECT ps.ps_partkey, ps.ps_availqty, ps.ps_supplycost FROM PartSupp ps",
            "SELECT ps.ps_suppkey, ps.ps_availqty FROM PartSupp ps",
            "SELECT ps.ps_partkey, ps.ps_comment FROM PartSupp ps",
            "SELECT ps.ps_supplycost, ps.ps_comment FROM PartSupp ps",
            "SELECT ps.ps_partkey, ps.ps_suppkey, ps.ps_availqty FROM PartSupp ps"
        },
        true
    });
    
    // Test Case 21-30: More complex scenarios
    testcases.push_back({21, "Supplier with Nation and Region",
        "SELECT s.s_name, n.n_name, r.r_name FROM Supplier s, Nation n, Region r WHERE s.s_nationkey = n.n_nationkey AND n.n_regionkey = r.r_regionkey",
        {
            "SELECT s.s_suppkey, s.s_name, s.s_nationkey FROM Supplier s",
            "SELECT n.n_nationkey, n.n_name, n.n_regionkey FROM Nation n",
            "SELECT r.r_regionkey, r.r_name FROM Region r",
            "SELECT s.s_name, s.s_address FROM Supplier s",
            "SELECT n.n_name, n.n_comment FROM Nation n",
            "SELECT r.r_name, r.r_comment FROM Region r",
            "SELECT s.s_nationkey, s.s_phone FROM Supplier s"
        },
        true
    });
    
    testcases.push_back({22, "Orders with date and price",
        "SELECT o.o_orderkey, o.o_orderdate, o.o_totalprice FROM Orders o",
        {
            "SELECT o.o_orderkey, o.o_custkey FROM Orders o",
            "SELECT o.o_orderdate, o.o_totalprice FROM Orders o",
            "SELECT o.o_orderkey, o.o_orderdate FROM Orders o",
            "SELECT o.o_orderkey, o.o_totalprice, o.o_orderstatus FROM Orders o",
            "SELECT o.o_totalprice, o.o_orderpriority FROM Orders o",
            "SELECT o.o_orderkey, o.o_orderdate, o.o_totalprice FROM Orders o"
        },
        true
    });
    
    testcases.push_back({23, "Customer with market segment",
        "SELECT c.c_name, c.c_mktsegment FROM Customer c",
        {
            "SELECT c.c_custkey, c.c_name FROM Customer c",
            "SELECT c.c_name, c.c_mktsegment, c.c_nationkey FROM Customer c",
            "SELECT c.c_mktsegment, c.c_address FROM Customer c",
            "SELECT c.c_name, c.c_phone FROM Customer c",
            "SELECT c.c_custkey, c.c_mktsegment FROM Customer c",
            "SELECT c.c_name, c.c_acctbal FROM Customer c"
        },
        true
    });
    
    testcases.push_back({24, "LineItem with extended price and discount",
        "SELECT l.l_orderkey, l.l_extendedprice, l.l_discount FROM LineItem l",
        {
            "SELECT l.l_orderkey, l.l_partkey FROM LineItem l",
            "SELECT l.l_extendedprice, l.l_discount, l.l_tax FROM LineItem l",
            "SELECT l.l_orderkey, l.l_extendedprice FROM LineItem l",
            "SELECT l.l_orderkey, l.l_discount, l.l_quantity FROM LineItem l",
            "SELECT l.l_extendedprice, l.l_returnflag FROM LineItem l",
            "SELECT l.l_orderkey, l.l_extendedprice, l.l_discount FROM LineItem l"
        },
        true
    });
    
    testcases.push_back({25, "Part with manufacturer and brand",
        "SELECT p.p_name, p.p_mfgr, p.p_brand FROM Part p",
        {
            "SELECT p.p_partkey, p.p_name FROM Part p",
            "SELECT p.p_mfgr, p.p_brand, p.p_type FROM Part p",
            "SELECT p.p_name, p.p_mfgr FROM Part p",
            "SELECT p.p_name, p.p_brand, p.p_size FROM Part p",
            "SELECT p.p_mfgr, p.p_retailprice FROM Part p",
            "SELECT p.p_name, p.p_mfgr, p.p_brand FROM Part p"
        },
        true
    });
    
    testcases.push_back({26, "All eight tables joined",
        "SELECT c.c_name, o.o_orderkey, l.l_quantity, p.p_name, s.s_name, n.n_name, r.r_name FROM Customer c, Orders o, LineItem l, Part p, Supplier s, Nation n, Region r, PartSupp ps WHERE c.c_custkey = o.o_custkey AND o.o_orderkey = l.l_orderkey AND l.l_partkey = p.p_partkey AND l.l_suppkey = s.s_suppkey AND s.s_nationkey = n.n_nationkey AND n.n_regionkey = r.r_regionkey AND p.p_partkey = ps.ps_partkey AND s.s_suppkey = ps.ps_suppkey",
        {
            "SELECT c.c_custkey, c.c_name FROM Customer c",
            "SELECT o.o_orderkey, o.o_custkey FROM Orders o",
            "SELECT l.l_orderkey, l.l_partkey, l.l_suppkey, l.l_quantity FROM LineItem l",
            "SELECT p.p_partkey, p.p_name FROM Part p",
            "SELECT s.s_suppkey, s.s_name, s.s_nationkey FROM Supplier s",
            "SELECT n.n_nationkey, n.n_name, n.n_regionkey FROM Nation n",
            "SELECT r.r_regionkey, r.r_name FROM Region r",
            "SELECT ps.ps_partkey, ps.ps_suppkey FROM PartSupp ps"
        },
        true
    });
    
    testcases.push_back({27, "Region details only",
        "SELECT r.r_name, r.r_comment FROM Region r",
        {
            "SELECT r.r_regionkey, r.r_name FROM Region r",
            "SELECT r.r_name, r.r_comment FROM Region r",
            "SELECT r.r_regionkey, r.r_comment FROM Region r",
            "SELECT r.r_name FROM Region r",
            "SELECT r.r_comment FROM Region r"
        },
        true
    });
    
    testcases.push_back({28, "Nation details with comments",
        "SELECT n.n_name, n.n_comment FROM Nation n",
        {
            "SELECT n.n_nationkey, n.n_name FROM Nation n",
            "SELECT n.n_name, n.n_regionkey FROM Nation n",
            "SELECT n.n_nationkey, n.n_comment FROM Nation n",
            "SELECT n.n_name, n.n_comment FROM Nation n",
            "SELECT n.n_regionkey, n.n_comment FROM Nation n",
            "SELECT n.n_name FROM Nation n"
        },
        true
    });
    
    testcases.push_back({29, "Supplier account balance",
        "SELECT s.s_name, s.s_acctbal FROM Supplier s",
        {
            "SELECT s.s_suppkey, s.s_name FROM Supplier s",
            "SELECT s.s_name, s.s_acctbal, s.s_phone FROM Supplier s",
            "SELECT s.s_acctbal, s.s_address FROM Supplier s",
            "SELECT s.s_suppkey, s.s_acctbal FROM Supplier s",
            "SELECT s.s_name, s.s_comment FROM Supplier s",
            "SELECT s.s_name, s.s_acctbal FROM Supplier s"
        },
        true
    });
    
    testcases.push_back({30, "Customer account details",
        "SELECT c.c_name, c.c_acctbal, c.c_phone FROM Customer c",
        {
            "SELECT c.c_custkey, c.c_name FROM Customer c",
            "SELECT c.c_acctbal, c.c_phone, c.c_mktsegment FROM Customer c",
            "SELECT c.c_name, c.c_acctbal FROM Customer c",
            "SELECT c.c_name, c.c_phone, c.c_address FROM Customer c",
            "SELECT c.c_custkey, c.c_phone FROM Customer c",
            "SELECT c.c_name, c.c_acctbal, c.c_phone FROM Customer c"
        },
        true
    });
    
    // Test Cases 31-40
    testcases.push_back({31, "Orders priority and status",
        "SELECT o.o_orderkey, o.o_orderpriority, o.o_orderstatus FROM Orders o",
        {
            "SELECT o.o_orderkey, o.o_custkey FROM Orders o",
            "SELECT o.o_orderpriority, o.o_orderstatus, o.o_clerk FROM Orders o",
            "SELECT o.o_orderkey, o.o_orderpriority FROM Orders o",
            "SELECT o.o_orderkey, o.o_orderstatus, o.o_totalprice FROM Orders o",
            "SELECT o.o_orderstatus, o.o_orderdate FROM Orders o",
            "SELECT o.o_orderkey, o.o_orderpriority, o.o_orderstatus FROM Orders o"
        },
        true
    });
    
    testcases.push_back({32, "LineItem shipping details",
        "SELECT l.l_orderkey, l.l_shipdate, l.l_shipmode FROM LineItem l",
        {
            "SELECT l.l_orderkey, l.l_linenumber FROM LineItem l",
            "SELECT l.l_shipdate, l.l_commitdate, l.l_receiptdate FROM LineItem l",
            "SELECT l.l_orderkey, l.l_shipdate FROM LineItem l",
            "SELECT l.l_orderkey, l.l_shipmode, l.l_shipinstruct FROM LineItem l",
            "SELECT l.l_shipmode, l.l_returnflag FROM LineItem l",
            "SELECT l.l_orderkey, l.l_shipdate, l.l_shipmode FROM LineItem l"
        },
        true
    });
    
    testcases.push_back({33, "Part container and size",
        "SELECT p.p_name, p.p_container, p.p_size FROM Part p",
        {
            "SELECT p.p_partkey, p.p_name FROM Part p",
            "SELECT p.p_container, p.p_size, p.p_retailprice FROM Part p",
            "SELECT p.p_name, p.p_container FROM Part p",
            "SELECT p.p_name, p.p_size, p.p_type FROM Part p",
            "SELECT p.p_partkey, p.p_container FROM Part p",
            "SELECT p.p_name, p.p_container, p.p_size FROM Part p"
        },
        true
    });
    
    testcases.push_back({34, "PartSupp supply cost",
        "SELECT ps.ps_partkey, ps.ps_suppkey, ps.ps_supplycost FROM PartSupp ps",
        {
            "SELECT ps.ps_partkey, ps.ps_suppkey FROM PartSupp ps",
            "SELECT ps.ps_supplycost, ps.ps_availqty FROM PartSupp ps",
            "SELECT ps.ps_partkey, ps.ps_supplycost FROM PartSupp ps",
            "SELECT ps.ps_suppkey, ps.ps_supplycost, ps.ps_comment FROM PartSupp ps",
            "SELECT ps.ps_partkey, ps.ps_suppkey, ps.ps_supplycost FROM PartSupp ps"
        },
        true
    });
    
    testcases.push_back({35, "Customer-Orders with total price",
        "SELECT c.c_name, o.o_orderkey, o.o_totalprice FROM Customer c, Orders o WHERE c.c_custkey = o.o_custkey",
        {
            "SELECT c.c_custkey, c.c_name, c.c_nationkey FROM Customer c",
            "SELECT o.o_orderkey, o.o_custkey, o.o_totalprice FROM Orders o",
            "SELECT c.c_name, c.c_address FROM Customer c",
            "SELECT o.o_totalprice, o.o_orderdate FROM Orders o",
            "SELECT c.c_custkey, c.c_mktsegment FROM Customer c",
            "SELECT o.o_orderkey, o.o_orderstatus FROM Orders o"
        },
        true
    });
    
    testcases.push_back({36, "LineItem-Part with retail price",
        "SELECT l.l_quantity, p.p_name, p.p_retailprice FROM LineItem l, Part p WHERE l.l_partkey = p.p_partkey",
        {
            "SELECT l.l_orderkey, l.l_partkey, l.l_quantity FROM LineItem l",
            "SELECT p.p_partkey, p.p_name, p.p_retailprice FROM Part p",
            "SELECT l.l_quantity, l.l_extendedprice FROM LineItem l",
            "SELECT p.p_name, p.p_type FROM Part p",
            "SELECT l.l_partkey, l.l_suppkey FROM LineItem l",
            "SELECT p.p_retailprice, p.p_size FROM Part p"
        },
        true
    });
    
    testcases.push_back({37, "Supplier-PartSupp with availability",
        "SELECT s.s_name, ps.ps_availqty FROM Supplier s, PartSupp ps WHERE s.s_suppkey = ps.ps_suppkey",
        {
            "SELECT s.s_suppkey, s.s_name, s.s_address FROM Supplier s",
            "SELECT ps.ps_partkey, ps.ps_suppkey, ps.ps_availqty FROM PartSupp ps",
            "SELECT s.s_name, s.s_nationkey FROM Supplier s",
            "SELECT ps.ps_availqty, ps.ps_supplycost FROM PartSupp ps",
            "SELECT s.s_suppkey, s.s_phone FROM Supplier s",
            "SELECT ps.ps_suppkey, ps.ps_comment FROM PartSupp ps"
        },
        true
    });
    
    testcases.push_back({38, "Orders-LineItem with discount",
        "SELECT o.o_orderkey, l.l_quantity, l.l_discount FROM Orders o, LineItem l WHERE o.o_orderkey = l.l_orderkey",
        {
            "SELECT o.o_orderkey, o.o_custkey, o.o_totalprice FROM Orders o",
            "SELECT l.l_orderkey, l.l_quantity, l.l_discount FROM LineItem l",
            "SELECT o.o_orderkey, o.o_orderdate FROM Orders o",
            "SELECT l.l_quantity, l.l_extendedprice FROM LineItem l",
            "SELECT o.o_orderkey, o.o_orderstatus FROM Orders o",
            "SELECT l.l_discount, l.l_tax FROM LineItem l"
        },
        true
    });
    
    testcases.push_back({39, "Customer-Nation with region key",
        "SELECT c.c_name, n.n_name, n.n_regionkey FROM Customer c, Nation n WHERE c.c_nationkey = n.n_nationkey",
        {
            "SELECT c.c_custkey, c.c_name, c.c_nationkey FROM Customer c",
            "SELECT n.n_nationkey, n.n_name, n.n_regionkey FROM Nation n",
            "SELECT c.c_name, c.c_address FROM Customer c",
            "SELECT n.n_name, n.n_comment FROM Nation n",
            "SELECT c.c_nationkey, c.c_phone FROM Customer c",
            "SELECT n.n_regionkey, n.n_comment FROM Nation n"
        },
        true
    });
    
    testcases.push_back({40, "Part-PartSupp with cost",
        "SELECT p.p_name, ps.ps_supplycost FROM Part p, PartSupp ps WHERE p.p_partkey = ps.ps_partkey",
        {
            "SELECT p.p_partkey, p.p_name, p.p_type FROM Part p",
            "SELECT ps.ps_partkey, ps.ps_suppkey, ps.ps_supplycost FROM PartSupp ps",
            "SELECT p.p_name, p.p_size FROM Part p",
            "SELECT ps.ps_supplycost, ps.ps_availqty FROM PartSupp ps",
            "SELECT p.p_partkey, p.p_retailprice FROM Part p",
            "SELECT ps.ps_partkey, ps.ps_comment FROM PartSupp ps"
        },
        true
    });
    
    // Test Cases 41-50
    testcases.push_back({41, "LineItem-Supplier with nation",
        "SELECT l.l_orderkey, s.s_name, s.s_nationkey FROM LineItem l, Supplier s WHERE l.l_suppkey = s.s_suppkey",
        {
            "SELECT l.l_orderkey, l.l_partkey, l.l_suppkey FROM LineItem l",
            "SELECT s.s_suppkey, s.s_name, s.s_nationkey FROM Supplier s",
            "SELECT l.l_orderkey, l.l_quantity FROM LineItem l",
            "SELECT s.s_name, s.s_address FROM Supplier s",
            "SELECT l.l_suppkey, l.l_extendedprice FROM LineItem l",
            "SELECT s.s_nationkey, s.s_phone FROM Supplier s"
        },
        true
    });
    
    testcases.push_back({42, "Customer with address and phone",
        "SELECT c.c_name, c.c_address, c.c_phone FROM Customer c",
        {
            "SELECT c.c_custkey, c.c_name FROM Customer c",
            "SELECT c.c_address, c.c_phone, c.c_nationkey FROM Customer c",
            "SELECT c.c_name, c.c_address FROM Customer c",
            "SELECT c.c_name, c.c_phone, c.c_mktsegment FROM Customer c",
            "SELECT c.c_custkey, c.c_address FROM Customer c",
            "SELECT c.c_name, c.c_address, c.c_phone FROM Customer c"
        },
        true
    });
    
    testcases.push_back({43, "Orders clerk and priority",
        "SELECT o.o_orderkey, o.o_clerk, o.o_orderpriority FROM Orders o",
        {
            "SELECT o.o_orderkey, o.o_custkey FROM Orders o",
            "SELECT o.o_clerk, o.o_orderpriority, o.o_shippriority FROM Orders o",
            "SELECT o.o_orderkey, o.o_clerk FROM Orders o",
            "SELECT o.o_orderkey, o.o_orderpriority, o.o_totalprice FROM Orders o",
            "SELECT o.o_clerk, o.o_orderdate FROM Orders o",
            "SELECT o.o_orderkey, o.o_clerk, o.o_orderpriority FROM Orders o"
        },
        true
    });
    
    testcases.push_back({44, "LineItem return flag and status",
        "SELECT l.l_orderkey, l.l_returnflag, l.l_linestatus FROM LineItem l",
        {
            "SELECT l.l_orderkey, l.l_linenumber FROM LineItem l",
            "SELECT l.l_returnflag, l.l_linestatus, l.l_shipdate FROM LineItem l",
            "SELECT l.l_orderkey, l.l_returnflag FROM LineItem l",
            "SELECT l.l_orderkey, l.l_linestatus, l.l_quantity FROM LineItem l",
            "SELECT l.l_returnflag, l.l_shipmode FROM LineItem l",
            "SELECT l.l_orderkey, l.l_returnflag, l.l_linestatus FROM LineItem l"
        },
        true
    });
    
    testcases.push_back({45, "Part type and brand details",
        "SELECT p.p_name, p.p_type, p.p_brand FROM Part p",
        {
            "SELECT p.p_partkey, p.p_name FROM Part p",
            "SELECT p.p_type, p.p_brand, p.p_mfgr FROM Part p",
            "SELECT p.p_name, p.p_type FROM Part p",
            "SELECT p.p_name, p.p_brand, p.p_size FROM Part p",
            "SELECT p.p_partkey, p.p_type FROM Part p",
            "SELECT p.p_name, p.p_type, p.p_brand FROM Part p"
        },
        true
    });
    
    testcases.push_back({46, "Supplier address and phone",
        "SELECT s.s_name, s.s_address, s.s_phone FROM Supplier s",
        {
            "SELECT s.s_suppkey, s.s_name FROM Supplier s",
            "SELECT s.s_address, s.s_phone, s.s_nationkey FROM Supplier s",
            "SELECT s.s_name, s.s_address FROM Supplier s",
            "SELECT s.s_name, s.s_phone, s.s_acctbal FROM Supplier s",
            "SELECT s.s_suppkey, s.s_address FROM Supplier s",
            "SELECT s.s_name, s.s_address, s.s_phone FROM Supplier s"
        },
        true
    });
    
    testcases.push_back({47, "Customer-Orders-Nation chain",
        "SELECT c.c_name, o.o_orderkey, n.n_name FROM Customer c, Orders o, Nation n WHERE c.c_custkey = o.o_custkey AND c.c_nationkey = n.n_nationkey",
        {
            "SELECT c.c_custkey, c.c_name, c.c_nationkey FROM Customer c",
            "SELECT o.o_orderkey, o.o_custkey FROM Orders o",
            "SELECT n.n_nationkey, n.n_name FROM Nation n",
            "SELECT c.c_name, c.c_address FROM Customer c",
            "SELECT o.o_orderkey, o.o_totalprice FROM Orders o",
            "SELECT n.n_name, n.n_regionkey FROM Nation n",
            "SELECT c.c_custkey, c.c_mktsegment FROM Customer c"
        },
        true
    });
    
    testcases.push_back({48, "LineItem-Part-PartSupp triangle",
        "SELECT l.l_orderkey, p.p_name, ps.ps_supplycost FROM LineItem l, Part p, PartSupp ps WHERE l.l_partkey = p.p_partkey AND p.p_partkey = ps.ps_partkey AND l.l_suppkey = ps.ps_suppkey",
        {
            "SELECT l.l_orderkey, l.l_partkey, l.l_suppkey FROM LineItem l",
            "SELECT p.p_partkey, p.p_name FROM Part p",
            "SELECT ps.ps_partkey, ps.ps_suppkey, ps.ps_supplycost FROM PartSupp ps",
            "SELECT l.l_orderkey, l.l_quantity FROM LineItem l",
            "SELECT p.p_name, p.p_type FROM Part p",
            "SELECT ps.ps_supplycost, ps.ps_availqty FROM PartSupp ps"
        },
        true
    });
    
    testcases.push_back({49, "Orders with shipping priority",
        "SELECT o.o_orderkey, o.o_shippriority FROM Orders o",
        {
            "SELECT o.o_orderkey, o.o_custkey FROM Orders o",
            "SELECT o.o_shippriority, o.o_orderpriority FROM Orders o",
            "SELECT o.o_orderkey, o.o_shippriority, o.o_totalprice FROM Orders o",
            "SELECT o.o_shippriority, o.o_orderdate FROM Orders o",
            "SELECT o.o_orderkey, o.o_orderstatus FROM Orders o",
            "SELECT o.o_orderkey, o.o_shippriority FROM Orders o"
        },
        true
    });
    
    testcases.push_back({50, "Nation-Region with comments",
        "SELECT n.n_name, r.r_name, r.r_comment FROM Nation n, Region r WHERE n.n_regionkey = r.r_regionkey",
        {
            "SELECT n.n_nationkey, n.n_name, n.n_regionkey FROM Nation n",
            "SELECT r.r_regionkey, r.r_name, r.r_comment FROM Region r",
            "SELECT n.n_name, n.n_comment FROM Nation n",
            "SELECT r.r_name, r.r_comment FROM Region r",
            "SELECT n.n_nationkey, n.n_regionkey FROM Nation n",
            "SELECT r.r_regionkey, r.r_comment FROM Region r"
        },
        true
    });
    
    // Test Cases 51-60
    testcases.push_back({51, "Customer market segment only",
        "SELECT c.c_custkey, c.c_mktsegment FROM Customer c",
        {
            "SELECT c.c_custkey, c.c_name FROM Customer c",
            "SELECT c.c_mktsegment, c.c_nationkey FROM Customer c",
            "SELECT c.c_custkey, c.c_mktsegment, c.c_address FROM Customer c",
            "SELECT c.c_mktsegment, c.c_phone FROM Customer c",
            "SELECT c.c_custkey, c.c_acctbal FROM Customer c",
            "SELECT c.c_custkey, c.c_mktsegment FROM Customer c"
        },
        true
    });
    
    testcases.push_back({52, "LineItem tax and discount",
        "SELECT l.l_orderkey, l.l_tax, l.l_discount FROM LineItem l",
        {
            "SELECT l.l_orderkey, l.l_partkey FROM LineItem l",
            "SELECT l.l_tax, l.l_discount, l.l_extendedprice FROM LineItem l",
            "SELECT l.l_orderkey, l.l_tax FROM LineItem l",
            "SELECT l.l_orderkey, l.l_discount, l.l_quantity FROM LineItem l",
            "SELECT l.l_tax, l.l_returnflag FROM LineItem l",
            "SELECT l.l_orderkey, l.l_tax, l.l_discount FROM LineItem l"
        },
        true
    });
    
    testcases.push_back({53, "Part retail price only",
        "SELECT p.p_partkey, p.p_retailprice FROM Part p",
        {
            "SELECT p.p_partkey, p.p_name FROM Part p",
            "SELECT p.p_retailprice, p.p_type FROM Part p",
            "SELECT p.p_partkey, p.p_retailprice, p.p_size FROM Part p",
            "SELECT p.p_retailprice, p.p_container FROM Part p",
            "SELECT p.p_partkey, p.p_mfgr FROM Part p",
            "SELECT p.p_partkey, p.p_retailprice FROM Part p"
        },
        true
    });
    
    testcases.push_back({54, "Supplier comment only",
        "SELECT s.s_suppkey, s.s_comment FROM Supplier s",
        {
            "SELECT s.s_suppkey, s.s_name FROM Supplier s",
            "SELECT s.s_comment, s.s_nationkey FROM Supplier s",
            "SELECT s.s_suppkey, s.s_comment, s.s_address FROM Supplier s",
            "SELECT s.s_comment, s.s_phone FROM Supplier s",
            "SELECT s.s_suppkey, s.s_acctbal FROM Supplier s",
            "SELECT s.s_suppkey, s.s_comment FROM Supplier s"
        },
        true
    });
    
    testcases.push_back({55, "Orders comment field",
        "SELECT o.o_orderkey, o.o_comment FROM Orders o",
        {
            "SELECT o.o_orderkey, o.o_custkey FROM Orders o",
            "SELECT o.o_comment, o.o_orderpriority FROM Orders o",
            "SELECT o.o_orderkey, o.o_comment, o.o_totalprice FROM Orders o",
            "SELECT o.o_comment, o.o_orderdate FROM Orders o",
            "SELECT o.o_orderkey, o.o_orderstatus FROM Orders o",
            "SELECT o.o_orderkey, o.o_comment FROM Orders o"
        },
        true
    });
    
    testcases.push_back({56, "Customer-Orders with order date",
        "SELECT c.c_name, o.o_orderkey, o.o_orderdate FROM Customer c, Orders o WHERE c.c_custkey = o.o_custkey",
        {
            "SELECT c.c_custkey, c.c_name, c.c_nationkey FROM Customer c",
            "SELECT o.o_orderkey, o.o_custkey, o.o_orderdate FROM Orders o",
            "SELECT c.c_name, c.c_address FROM Customer c",
            "SELECT o.o_orderdate, o.o_totalprice FROM Orders o",
            "SELECT c.c_custkey, c.c_mktsegment FROM Customer c",
            "SELECT o.o_orderkey, o.o_orderstatus FROM Orders o"
        },
        true
    });
    
    testcases.push_back({57, "LineItem commit and receipt dates",
        "SELECT l.l_orderkey, l.l_commitdate, l.l_receiptdate FROM LineItem l",
        {
            "SELECT l.l_orderkey, l.l_partkey FROM LineItem l",
            "SELECT l.l_commitdate, l.l_receiptdate, l.l_shipdate FROM LineItem l",
            "SELECT l.l_orderkey, l.l_commitdate FROM LineItem l",
            "SELECT l.l_orderkey, l.l_receiptdate, l.l_quantity FROM LineItem l",
            "SELECT l.l_commitdate, l.l_shipmode FROM LineItem l",
            "SELECT l.l_orderkey, l.l_commitdate, l.l_receiptdate FROM LineItem l"
        },
        true
    });
    
    testcases.push_back({58, "Part comment field",
        "SELECT p.p_partkey, p.p_comment FROM Part p",
        {
            "SELECT p.p_partkey, p.p_name FROM Part p",
            "SELECT p.p_comment, p.p_type FROM Part p",
            "SELECT p.p_partkey, p.p_comment, p.p_size FROM Part p",
            "SELECT p.p_comment, p.p_retailprice FROM Part p",
            "SELECT p.p_partkey, p.p_mfgr FROM Part p",
            "SELECT p.p_partkey, p.p_comment FROM Part p"
        },
        true
    });
    
    testcases.push_back({59, "PartSupp comment field",
        "SELECT ps.ps_partkey, ps.ps_suppkey, ps.ps_comment FROM PartSupp ps",
        {
            "SELECT ps.ps_partkey, ps.ps_suppkey FROM PartSupp ps",
            "SELECT ps.ps_comment, ps.ps_availqty FROM PartSupp ps",
            "SELECT ps.ps_partkey, ps.ps_comment FROM PartSupp ps",
            "SELECT ps.ps_suppkey, ps.ps_comment, ps.ps_supplycost FROM PartSupp ps",
            "SELECT ps.ps_partkey, ps.ps_availqty FROM PartSupp ps",
            "SELECT ps.ps_partkey, ps.ps_suppkey, ps.ps_comment FROM PartSupp ps"
        },
        true
    });
    
    testcases.push_back({60, "LineItem ship instruction",
        "SELECT l.l_orderkey, l.l_shipinstruct FROM LineItem l",
        {
            "SELECT l.l_orderkey, l.l_partkey FROM LineItem l",
            "SELECT l.l_shipinstruct, l.l_shipmode FROM LineItem l",
            "SELECT l.l_orderkey, l.l_shipinstruct, l.l_shipdate FROM LineItem l",
            "SELECT l.l_shipinstruct, l.l_quantity FROM LineItem l",
            "SELECT l.l_orderkey, l.l_suppkey FROM LineItem l",
            "SELECT l.l_orderkey, l.l_shipinstruct FROM LineItem l"
        },
        true
    });
    
    // Test Cases 61-70
    testcases.push_back({61, "Customer-Nation-Orders three-way",
        "SELECT c.c_name, n.n_name, o.o_orderkey FROM Customer c, Nation n, Orders o WHERE c.c_nationkey = n.n_nationkey AND c.c_custkey = o.o_custkey",
        {
            "SELECT c.c_custkey, c.c_name, c.c_nationkey FROM Customer c",
            "SELECT n.n_nationkey, n.n_name FROM Nation n",
            "SELECT o.o_orderkey, o.o_custkey FROM Orders o",
            "SELECT c.c_name, c.c_address FROM Customer c",
            "SELECT n.n_name, n.n_regionkey FROM Nation n",
            "SELECT o.o_orderkey, o.o_totalprice FROM Orders o",
            "SELECT c.c_custkey, c.c_phone FROM Customer c"
        },
        true
    });
    
    testcases.push_back({62, "Supplier-Nation-PartSupp chain",
        "SELECT s.s_name, n.n_name, ps.ps_availqty FROM Supplier s, Nation n, PartSupp ps WHERE s.s_nationkey = n.n_nationkey AND s.s_suppkey = ps.ps_suppkey",
        {
            "SELECT s.s_suppkey, s.s_name, s.s_nationkey FROM Supplier s",
            "SELECT n.n_nationkey, n.n_name FROM Nation n",
            "SELECT ps.ps_partkey, ps.ps_suppkey, ps.ps_availqty FROM PartSupp ps",
            "SELECT s.s_name, s.s_address FROM Supplier s",
            "SELECT n.n_name, n.n_regionkey FROM Nation n",
            "SELECT ps.ps_availqty, ps.ps_supplycost FROM PartSupp ps"
        },
        true
    });
    
    testcases.push_back({63, "Part-PartSupp-Supplier chain",
        "SELECT p.p_name, ps.ps_availqty, s.s_name FROM Part p, PartSupp ps, Supplier s WHERE p.p_partkey = ps.ps_partkey AND ps.ps_suppkey = s.s_suppkey",
        {
            "SELECT p.p_partkey, p.p_name FROM Part p",
            "SELECT ps.ps_partkey, ps.ps_suppkey, ps.ps_availqty FROM PartSupp ps",
            "SELECT s.s_suppkey, s.s_name FROM Supplier s",
            "SELECT p.p_name, p.p_type FROM Part p",
            "SELECT ps.ps_availqty, ps.ps_supplycost FROM PartSupp ps",
            "SELECT s.s_name, s.s_nationkey FROM Supplier s",
            "SELECT p.p_partkey, p.p_size FROM Part p"
        },
        true
    });
    
    testcases.push_back({64, "Orders-LineItem-Part chain",
        "SELECT o.o_orderkey, l.l_quantity, p.p_name FROM Orders o, LineItem l, Part p WHERE o.o_orderkey = l.l_orderkey AND l.l_partkey = p.p_partkey",
        {
            "SELECT o.o_orderkey, o.o_custkey FROM Orders o",
            "SELECT l.l_orderkey, l.l_partkey, l.l_quantity FROM LineItem l",
            "SELECT p.p_partkey, p.p_name FROM Part p",
            "SELECT o.o_orderkey, o.o_totalprice FROM Orders o",
            "SELECT l.l_quantity, l.l_extendedprice FROM LineItem l",
            "SELECT p.p_name, p.p_type FROM Part p"
        },
        true
    });
    
    testcases.push_back({65, "Customer acctbal only",
        "SELECT c.c_custkey, c.c_acctbal FROM Customer c",
        {
            "SELECT c.c_custkey, c.c_name FROM Customer c",
            "SELECT c.c_acctbal, c.c_nationkey FROM Customer c",
            "SELECT c.c_custkey, c.c_acctbal, c.c_mktsegment FROM Customer c",
            "SELECT c.c_acctbal, c.c_phone FROM Customer c",
            "SELECT c.c_custkey, c.c_address FROM Customer c",
            "SELECT c.c_custkey, c.c_acctbal FROM Customer c"
        },
        true
    });
    
    testcases.push_back({66, "Orders orderdate only",
        "SELECT o.o_orderkey, o.o_orderdate FROM Orders o",
        {
            "SELECT o.o_orderkey, o.o_custkey FROM Orders o",
            "SELECT o.o_orderdate, o.o_totalprice FROM Orders o",
            "SELECT o.o_orderkey, o.o_orderdate, o.o_orderpriority FROM Orders o",
            "SELECT o.o_orderdate, o.o_orderstatus FROM Orders o",
            "SELECT o.o_orderkey, o.o_clerk FROM Orders o",
            "SELECT o.o_orderkey, o.o_orderdate FROM Orders o"
        },
        true
    });
    
    testcases.push_back({67, "LineItem linenumber field",
        "SELECT l.l_orderkey, l.l_linenumber FROM LineItem l",
        {
            "SELECT l.l_orderkey, l.l_partkey FROM LineItem l",
            "SELECT l.l_linenumber, l.l_quantity FROM LineItem l",
            "SELECT l.l_orderkey, l.l_linenumber, l.l_suppkey FROM LineItem l",
            "SELECT l.l_linenumber, l.l_extendedprice FROM LineItem l",
            "SELECT l.l_orderkey, l.l_discount FROM LineItem l",
            "SELECT l.l_orderkey, l.l_linenumber FROM LineItem l"
        },
        true
    });
    
    testcases.push_back({68, "Part manufacturer only",
        "SELECT p.p_partkey, p.p_mfgr FROM Part p",
        {
            "SELECT p.p_partkey, p.p_name FROM Part p",
            "SELECT p.p_mfgr, p.p_brand FROM Part p",
            "SELECT p.p_partkey, p.p_mfgr, p.p_type FROM Part p",
            "SELECT p.p_mfgr, p.p_size FROM Part p",
            "SELECT p.p_partkey, p.p_retailprice FROM Part p",
            "SELECT p.p_partkey, p.p_mfgr FROM Part p"
        },
        true
    });
    
    testcases.push_back({69, "Nation regionkey only",
        "SELECT n.n_nationkey, n.n_regionkey FROM Nation n",
        {
            "SELECT n.n_nationkey, n.n_name FROM Nation n",
            "SELECT n.n_regionkey, n.n_comment FROM Nation n",
            "SELECT n.n_nationkey, n.n_regionkey, n.n_name FROM Nation n",
            "SELECT n.n_regionkey FROM Nation n",
            "SELECT n.n_nationkey, n.n_comment FROM Nation n",
            "SELECT n.n_nationkey, n.n_regionkey FROM Nation n"
        },
        true
    });
    
    testcases.push_back({70, "Region regionkey only",
        "SELECT r.r_regionkey FROM Region r",
        {
            "SELECT r.r_regionkey, r.r_name FROM Region r",
            "SELECT r.r_regionkey, r.r_comment FROM Region r",
            "SELECT r.r_regionkey FROM Region r",
            "SELECT r.r_name FROM Region r"
        },
        true
    });
    
    // Test Cases 71-80
    testcases.push_back({71, "Customer-Orders-LineItem-Supplier four-way",
        "SELECT c.c_name, o.o_orderkey, l.l_quantity, s.s_name FROM Customer c, Orders o, LineItem l, Supplier s WHERE c.c_custkey = o.o_custkey AND o.o_orderkey = l.l_orderkey AND l.l_suppkey = s.s_suppkey",
        {
            "SELECT c.c_custkey, c.c_name FROM Customer c",
            "SELECT o.o_orderkey, o.o_custkey FROM Orders o",
            "SELECT l.l_orderkey, l.l_suppkey, l.l_quantity FROM LineItem l",
            "SELECT s.s_suppkey, s.s_name FROM Supplier s",
            "SELECT c.c_name, c.c_nationkey FROM Customer c",
            "SELECT o.o_orderkey, o.o_totalprice FROM Orders o",
            "SELECT l.l_quantity, l.l_extendedprice FROM LineItem l",
            "SELECT s.s_name, s.s_address FROM Supplier s"
        },
        true
    });
    
    testcases.push_back({72, "Incomplete view - missing head variable",
        "SELECT c.c_name, c.c_phone FROM Customer c",
        {
            "SELECT c.c_custkey, c.c_name FROM Customer c",
            "SELECT c.c_address, c.c_nationkey FROM Customer c",
            "SELECT c.c_mktsegment, c.c_acctbal FROM Customer c",
            "SELECT c.c_custkey, c.c_comment FROM Customer c",
            "SELECT c.c_name, c.c_address FROM Customer c"
        },
        true
    });
    
    testcases.push_back({73, "Orders totalprice and status",
        "SELECT o.o_orderkey, o.o_totalprice, o.o_orderstatus FROM Orders o",
        {
            "SELECT o.o_orderkey, o.o_custkey FROM Orders o",
            "SELECT o.o_totalprice, o.o_orderstatus FROM Orders o",
            "SELECT o.o_orderkey, o.o_totalprice FROM Orders o",
            "SELECT o.o_orderkey, o.o_orderstatus, o.o_orderdate FROM Orders o",
            "SELECT o.o_totalprice, o.o_orderpriority FROM Orders o",
            "SELECT o.o_orderkey, o.o_totalprice, o.o_orderstatus FROM Orders o"
        },
        true
    });
    
    testcases.push_back({74, "LineItem with all price fields",
        "SELECT l.l_orderkey, l.l_extendedprice, l.l_discount, l.l_tax FROM LineItem l",
        {
            "SELECT l.l_orderkey, l.l_partkey FROM LineItem l",
            "SELECT l.l_extendedprice, l.l_discount, l.l_tax FROM LineItem l",
            "SELECT l.l_orderkey, l.l_extendedprice FROM LineItem l",
            "SELECT l.l_orderkey, l.l_discount, l.l_tax FROM LineItem l",
            "SELECT l.l_extendedprice, l.l_quantity FROM LineItem l",
            "SELECT l.l_orderkey, l.l_extendedprice, l.l_discount, l.l_tax FROM LineItem l"
        },
        true
    });
    
    testcases.push_back({75, "Part with all description fields",
        "SELECT p.p_name, p.p_mfgr, p.p_brand, p.p_type FROM Part p",
        {
            "SELECT p.p_partkey, p.p_name FROM Part p",
            "SELECT p.p_mfgr, p.p_brand, p.p_type FROM Part p",
            "SELECT p.p_name, p.p_mfgr FROM Part p",
            "SELECT p.p_name, p.p_brand, p.p_type FROM Part p",
            "SELECT p.p_mfgr, p.p_type, p.p_size FROM Part p",
            "SELECT p.p_name, p.p_mfgr, p.p_brand, p.p_type FROM Part p"
        },
        true
    });
    
    testcases.push_back({76, "Supplier full contact info",
        "SELECT s.s_name, s.s_address, s.s_phone, s.s_acctbal FROM Supplier s",
        {
            "SELECT s.s_suppkey, s.s_name FROM Supplier s",
            "SELECT s.s_address, s.s_phone, s.s_acctbal FROM Supplier s",
            "SELECT s.s_name, s.s_address FROM Supplier s",
            "SELECT s.s_name, s.s_phone, s.s_nationkey FROM Supplier s",
            "SELECT s.s_address, s.s_acctbal FROM Supplier s",
            "SELECT s.s_name, s.s_address, s.s_phone, s.s_acctbal FROM Supplier s"
        },
        true
    });
    
    testcases.push_back({77, "Customer full contact info",
        "SELECT c.c_name, c.c_address, c.c_phone, c.c_acctbal FROM Customer c",
        {
            "SELECT c.c_custkey, c.c_name FROM Customer c",
            "SELECT c.c_address, c.c_phone, c.c_acctbal FROM Customer c",
            "SELECT c.c_name, c.c_address FROM Customer c",
            "SELECT c.c_name, c.c_phone, c.c_mktsegment FROM Customer c",
            "SELECT c.c_address, c.c_acctbal FROM Customer c",
            "SELECT c.c_name, c.c_address, c.c_phone, c.c_acctbal FROM Customer c"
        },
        true
    });
    
    testcases.push_back({78, "Orders full details",
        "SELECT o.o_orderkey, o.o_totalprice, o.o_orderdate, o.o_orderpriority FROM Orders o",
        {
            "SELECT o.o_orderkey, o.o_custkey FROM Orders o",
            "SELECT o.o_totalprice, o.o_orderdate, o.o_orderpriority FROM Orders o",
            "SELECT o.o_orderkey, o.o_totalprice FROM Orders o",
            "SELECT o.o_orderkey, o.o_orderdate, o.o_orderpriority FROM Orders o",
            "SELECT o.o_totalprice, o.o_orderstatus FROM Orders o",
            "SELECT o.o_orderkey, o.o_totalprice, o.o_orderdate, o.o_orderpriority FROM Orders o"
        },
        true
    });
    
    testcases.push_back({79, "LineItem full shipping info",
        "SELECT l.l_orderkey, l.l_shipdate, l.l_commitdate, l.l_receiptdate FROM LineItem l",
        {
            "SELECT l.l_orderkey, l.l_partkey FROM LineItem l",
            "SELECT l.l_shipdate, l.l_commitdate, l.l_receiptdate FROM LineItem l",
            "SELECT l.l_orderkey, l.l_shipdate FROM LineItem l",
            "SELECT l.l_orderkey, l.l_commitdate, l.l_receiptdate FROM LineItem l",
            "SELECT l.l_shipdate, l.l_shipmode FROM LineItem l",
            "SELECT l.l_orderkey, l.l_shipdate, l.l_commitdate, l.l_receiptdate FROM LineItem l"
        },
        true
    });
    
    testcases.push_back({80, "Part size and container",
        "SELECT p.p_name, p.p_size, p.p_container FROM Part p",
        {
            "SELECT p.p_partkey, p.p_name FROM Part p",
            "SELECT p.p_size, p.p_container, p.p_retailprice FROM Part p",
            "SELECT p.p_name, p.p_size FROM Part p",
            "SELECT p.p_name, p.p_container, p.p_type FROM Part p",
            "SELECT p.p_partkey, p.p_size FROM Part p",
            "SELECT p.p_name, p.p_size, p.p_container FROM Part p"
        },
        true
    });
    
    // Test Cases 81-90
    testcases.push_back({81, "Complex six-table join",
        "SELECT c.c_name, o.o_orderkey, l.l_quantity, p.p_name, s.s_name, n.n_name FROM Customer c, Orders o, LineItem l, Part p, Supplier s, Nation n WHERE c.c_custkey = o.o_custkey AND o.o_orderkey = l.l_orderkey AND l.l_partkey = p.p_partkey AND l.l_suppkey = s.s_suppkey AND s.s_nationkey = n.n_nationkey",
        {
            "SELECT c.c_custkey, c.c_name FROM Customer c",
            "SELECT o.o_orderkey, o.o_custkey FROM Orders o",
            "SELECT l.l_orderkey, l.l_partkey, l.l_suppkey, l.l_quantity FROM LineItem l",
            "SELECT p.p_partkey, p.p_name FROM Part p",
            "SELECT s.s_suppkey, s.s_name, s.s_nationkey FROM Supplier s",
            "SELECT n.n_nationkey, n.n_name FROM Nation n",
            "SELECT c.c_name, c.c_address FROM Customer c",
            "SELECT o.o_orderkey, o.o_totalprice FROM Orders o"
        },
        true
    });
    
    testcases.push_back({82, "PartSupp with both keys",
        "SELECT ps.ps_partkey, ps.ps_suppkey FROM PartSupp ps",
        {
            "SELECT ps.ps_partkey, ps.ps_suppkey, ps.ps_availqty FROM PartSupp ps",
            "SELECT ps.ps_partkey, ps.ps_supplycost FROM PartSupp ps",
            "SELECT ps.ps_suppkey, ps.ps_comment FROM PartSupp ps",
            "SELECT ps.ps_partkey, ps.ps_suppkey FROM PartSupp ps",
            "SELECT ps.ps_availqty, ps.ps_supplycost FROM PartSupp ps"
        },
        true
    });
    
    testcases.push_back({83, "Orders with clerk info",
        "SELECT o.o_orderkey, o.o_clerk FROM Orders o",
        {
            "SELECT o.o_orderkey, o.o_custkey FROM Orders o",
            "SELECT o.o_clerk, o.o_orderpriority FROM Orders o",
            "SELECT o.o_orderkey, o.o_clerk, o.o_totalprice FROM Orders o",
            "SELECT o.o_clerk, o.o_orderdate FROM Orders o",
            "SELECT o.o_orderkey, o.o_orderstatus FROM Orders o",
            "SELECT o.o_orderkey, o.o_clerk FROM Orders o"
        },
        true
    });
    
    testcases.push_back({84, "LineItem quantity and price",
        "SELECT l.l_orderkey, l.l_quantity, l.l_extendedprice FROM LineItem l",
        {
            "SELECT l.l_orderkey, l.l_partkey FROM LineItem l",
            "SELECT l.l_quantity, l.l_extendedprice, l.l_discount FROM LineItem l",
            "SELECT l.l_orderkey, l.l_quantity FROM LineItem l",
            "SELECT l.l_orderkey, l.l_extendedprice, l.l_tax FROM LineItem l",
            "SELECT l.l_quantity, l.l_suppkey FROM LineItem l",
            "SELECT l.l_orderkey, l.l_quantity, l.l_extendedprice FROM LineItem l"
        },
        true
    });
    
    testcases.push_back({85, "Part name and type only",
        "SELECT p.p_name, p.p_type FROM Part p",
        {
            "SELECT p.p_partkey, p.p_name FROM Part p",
            "SELECT p.p_type, p.p_size FROM Part p",
            "SELECT p.p_name, p.p_type, p.p_brand FROM Part p",
            "SELECT p.p_type, p.p_mfgr FROM Part p",
            "SELECT p.p_partkey, p.p_type FROM Part p",
            "SELECT p.p_name, p.p_type FROM Part p"
        },
        true
    });
    
    testcases.push_back({86, "Supplier nationkey only",
        "SELECT s.s_suppkey, s.s_nationkey FROM Supplier s",
        {
            "SELECT s.s_suppkey, s.s_name FROM Supplier s",
            "SELECT s.s_nationkey, s.s_address FROM Supplier s",
            "SELECT s.s_suppkey, s.s_nationkey, s.s_phone FROM Supplier s",
            "SELECT s.s_nationkey, s.s_acctbal FROM Supplier s",
            "SELECT s.s_suppkey, s.s_comment FROM Supplier s",
            "SELECT s.s_suppkey, s.s_nationkey FROM Supplier s"
        },
        true
    });
    
    testcases.push_back({87, "Customer nationkey only",
        "SELECT c.c_custkey, c.c_nationkey FROM Customer c",
        {
            "SELECT c.c_custkey, c.c_name FROM Customer c",
            "SELECT c.c_nationkey, c.c_address FROM Customer c",
            "SELECT c.c_custkey, c.c_nationkey, c.c_phone FROM Customer c",
            "SELECT c.c_nationkey, c.c_mktsegment FROM Customer c",
            "SELECT c.c_custkey, c.c_acctbal FROM Customer c",
            "SELECT c.c_custkey, c.c_nationkey FROM Customer c"
        },
        true
    });
    
    testcases.push_back({88, "Orders custkey only",
        "SELECT o.o_orderkey, o.o_custkey FROM Orders o",
        {
            "SELECT o.o_orderkey, o.o_custkey, o.o_totalprice FROM Orders o",
            "SELECT o.o_custkey, o.o_orderdate FROM Orders o",
            "SELECT o.o_orderkey, o.o_orderstatus FROM Orders o",
            "SELECT o.o_orderkey, o.o_custkey FROM Orders o",
            "SELECT o.o_custkey, o.o_orderpriority FROM Orders o"
        },
        true
    });
    
    testcases.push_back({89, "LineItem with suppkey and partkey",
        "SELECT l.l_orderkey, l.l_partkey, l.l_suppkey FROM LineItem l",
        {
            "SELECT l.l_orderkey, l.l_partkey, l.l_suppkey, l.l_quantity FROM LineItem l",
            "SELECT l.l_partkey, l.l_suppkey FROM LineItem l",
            "SELECT l.l_orderkey, l.l_partkey FROM LineItem l",
            "SELECT l.l_orderkey, l.l_suppkey, l.l_extendedprice FROM LineItem l",
            "SELECT l.l_partkey, l.l_linenumber FROM LineItem l",
            "SELECT l.l_orderkey, l.l_partkey, l.l_suppkey FROM LineItem l"
        },
        true
    });
    
    testcases.push_back({90, "Nation all fields",
        "SELECT n.n_nationkey, n.n_name, n.n_regionkey, n.n_comment FROM Nation n",
        {
            "SELECT n.n_nationkey, n.n_name FROM Nation n",
            "SELECT n.n_name, n.n_regionkey, n.n_comment FROM Nation n",
            "SELECT n.n_nationkey, n.n_regionkey FROM Nation n",
            "SELECT n.n_nationkey, n.n_name, n.n_comment FROM Nation n",
            "SELECT n.n_regionkey, n.n_comment FROM Nation n",
            "SELECT n.n_nationkey, n.n_name, n.n_regionkey, n.n_comment FROM Nation n"
        },
        true
    });
    
    // Test Cases 91-100
    testcases.push_back({91, "Region all fields",
        "SELECT r.r_regionkey, r.r_name, r.r_comment FROM Region r",
        {
            "SELECT r.r_regionkey, r.r_name FROM Region r",
            "SELECT r.r_name, r.r_comment FROM Region r",
            "SELECT r.r_regionkey, r.r_comment FROM Region r",
            "SELECT r.r_regionkey, r.r_name, r.r_comment FROM Region r",
            "SELECT r.r_name FROM Region r"
        },
        true
    });
    
    testcases.push_back({92, "Customer-Orders with status",
        "SELECT c.c_name, o.o_orderkey, o.o_orderstatus FROM Customer c, Orders o WHERE c.c_custkey = o.o_custkey",
        {
            "SELECT c.c_custkey, c.c_name FROM Customer c",
            "SELECT o.o_orderkey, o.o_custkey, o.o_orderstatus FROM Orders o",
            "SELECT c.c_name, c.c_nationkey FROM Customer c",
            "SELECT o.o_orderstatus, o.o_totalprice FROM Orders o",
            "SELECT c.c_custkey, c.c_address FROM Customer c",
            "SELECT o.o_orderkey, o.o_orderdate FROM Orders o"
        },
        true
    });
    
    testcases.push_back({93, "LineItem-Supplier simple join",
        "SELECT l.l_orderkey, s.s_name FROM LineItem l, Supplier s WHERE l.l_suppkey = s.s_suppkey",
        {
            "SELECT l.l_orderkey, l.l_partkey, l.l_suppkey FROM LineItem l",
            "SELECT s.s_suppkey, s.s_name FROM Supplier s",
            "SELECT l.l_orderkey, l.l_quantity FROM LineItem l",
            "SELECT s.s_name, s.s_address FROM Supplier s",
            "SELECT l.l_suppkey, l.l_extendedprice FROM LineItem l",
            "SELECT s.s_suppkey, s.s_nationkey FROM Supplier s"
        },
        true
    });
    
    testcases.push_back({94, "Part-PartSupp simple join",
        "SELECT p.p_name, ps.ps_availqty FROM Part p, PartSupp ps WHERE p.p_partkey = ps.ps_partkey",
        {
            "SELECT p.p_partkey, p.p_name FROM Part p",
            "SELECT ps.ps_partkey, ps.ps_suppkey, ps.ps_availqty FROM PartSupp ps",
            "SELECT p.p_name, p.p_type FROM Part p",
            "SELECT ps.ps_availqty, ps.ps_supplycost FROM PartSupp ps",
            "SELECT p.p_partkey, p.p_size FROM Part p",
            "SELECT ps.ps_partkey, ps.ps_comment FROM PartSupp ps"
        },
        true
    });
    
    testcases.push_back({95, "Orders-LineItem with tax",
        "SELECT o.o_orderkey, l.l_quantity, l.l_tax FROM Orders o, LineItem l WHERE o.o_orderkey = l.l_orderkey",
        {
            "SELECT o.o_orderkey, o.o_custkey FROM Orders o",
            "SELECT l.l_orderkey, l.l_quantity, l.l_tax FROM LineItem l",
            "SELECT o.o_orderkey, o.o_totalprice FROM Orders o",
            "SELECT l.l_quantity, l.l_discount FROM LineItem l",
            "SELECT o.o_orderkey, o.o_orderdate FROM Orders o",
            "SELECT l.l_tax, l.l_extendedprice FROM LineItem l"
        },
        true
    });
    
    testcases.push_back({96, "Customer-Nation-Region full chain",
        "SELECT c.c_name, n.n_name, r.r_name FROM Customer c, Nation n, Region r WHERE c.c_nationkey = n.n_nationkey AND n.n_regionkey = r.r_regionkey",
        {
            "SELECT c.c_custkey, c.c_name, c.c_nationkey FROM Customer c",
            "SELECT n.n_nationkey, n.n_name, n.n_regionkey FROM Nation n",
            "SELECT r.r_regionkey, r.r_name FROM Region r",
            "SELECT c.c_name, c.c_address FROM Customer c",
            "SELECT n.n_name, n.n_comment FROM Nation n",
            "SELECT r.r_name, r.r_comment FROM Region r",
            "SELECT c.c_custkey, c.c_phone FROM Customer c"
        },
        true
    });
    
    testcases.push_back({97, "Supplier-Nation-Region full chain",
        "SELECT s.s_name, n.n_name, r.r_name FROM Supplier s, Nation n, Region r WHERE s.s_nationkey = n.n_nationkey AND n.n_regionkey = r.r_regionkey",
        {
            "SELECT s.s_suppkey, s.s_name, s.s_nationkey FROM Supplier s",
            "SELECT n.n_nationkey, n.n_name, n.n_regionkey FROM Nation n",
            "SELECT r.r_regionkey, r.r_name FROM Region r",
            "SELECT s.s_name, s.s_address FROM Supplier s",
            "SELECT n.n_name, n.n_comment FROM Nation n",
            "SELECT r.r_name, r.r_comment FROM Region r",
            "SELECT s.s_suppkey, s.s_phone FROM Supplier s"
        },
        true
    });
    
    testcases.push_back({98, "LineItem with all status fields",
        "SELECT l.l_orderkey, l.l_returnflag, l.l_linestatus FROM LineItem l",
        {
            "SELECT l.l_orderkey, l.l_partkey FROM LineItem l",
            "SELECT l.l_returnflag, l.l_linestatus FROM LineItem l",
            "SELECT l.l_orderkey, l.l_returnflag, l.l_quantity FROM LineItem l",
            "SELECT l.l_orderkey, l.l_linestatus, l.l_shipdate FROM LineItem l",
            "SELECT l.l_returnflag, l.l_shipmode FROM LineItem l",
            "SELECT l.l_orderkey, l.l_returnflag, l.l_linestatus FROM LineItem l"
        },
        true
    });
    
    testcases.push_back({99, "Part brand and size",
        "SELECT p.p_name, p.p_brand, p.p_size FROM Part p",
        {
            "SELECT p.p_partkey, p.p_name FROM Part p",
            "SELECT p.p_brand, p.p_size, p.p_type FROM Part p",
            "SELECT p.p_name, p.p_brand FROM Part p",
            "SELECT p.p_name, p.p_size, p.p_container FROM Part p",
            "SELECT p.p_partkey, p.p_brand FROM Part p",
            "SELECT p.p_name, p.p_brand, p.p_size FROM Part p"
        },
        true
    });
    
    testcases.push_back({100, "Seven-table mega join",
        "SELECT c.c_name, o.o_orderkey, l.l_quantity, p.p_name, s.s_name, n.n_name, r.r_name FROM Customer c, Orders o, LineItem l, Part p, Supplier s, Nation n, Region r WHERE c.c_custkey = o.o_custkey AND o.o_orderkey = l.l_orderkey AND l.l_partkey = p.p_partkey AND l.l_suppkey = s.s_suppkey AND c.c_nationkey = n.n_nationkey AND n.n_regionkey = r.r_regionkey",
        {
            "SELECT c.c_custkey, c.c_name, c.c_nationkey FROM Customer c",
            "SELECT o.o_orderkey, o.o_custkey FROM Orders o",
            "SELECT l.l_orderkey, l.l_partkey, l.l_suppkey, l.l_quantity FROM LineItem l",
            "SELECT p.p_partkey, p.p_name FROM Part p",
            "SELECT s.s_suppkey, s.s_name FROM Supplier s",
            "SELECT n.n_nationkey, n.n_name, n.n_regionkey FROM Nation n",
            "SELECT r.r_regionkey, r.r_name FROM Region r",
            "SELECT c.c_name, c.c_address FROM Customer c"
        },
        true
    });
    
    return testcases;
}

void writeTestCasesToFile(const std::vector<TestCase>& testcases, const std::string& filename) {
    std::ofstream outfile(filename);
    
    if (!outfile.is_open()) {
        std::cerr << "Error: Could not open file " << filename << " for writing.\n";
        return;
    }
    
    outfile << "# MiniCon Algorithm Test Cases\n";
    outfile << "# TPC-H Schema Based\n";
    outfile << "# Total Test Cases: " << testcases.size() << "\n\n";
    
    for (const auto& tc : testcases) {
        outfile << "## Test Case " << tc.id << ": " << tc.description << "\n";
        outfile << "Expected: " << (tc.should_have_rewriting ? "REWRITING EXISTS" : "NO REWRITING") << "\n\n";
        
        outfile << "Query:\n";
        outfile << tc.query << "\n\n";
        
        outfile << "Views (" << tc.views.size() << "):\n";
        for (size_t i = 0; i < tc.views.size(); ++i) {
            outfile << "V" << i << ": " << tc.views[i] << "\n";
        }
        
        outfile << "\n" << std::string(80, '-') << "\n\n";
    }
    
    outfile.close();
    std::cout << "Test cases written to " << filename << "\n";
}

int main() {
    std::vector<TestCase> testcases = generateTestCases();
    
    std::cout << "Generated " << testcases.size() << " test cases for MiniCon algorithm.\n";
    std::cout << "All test cases use TPC-H schema.\n\n";
    
    // Write to file
    writeTestCasesToFile(testcases, "minicon_testcases.txt");
    
    // Display summary
    int with_rewriting = 0;
    int without_rewriting = 0;
    
    for (const auto& tc : testcases) {
        if (tc.should_have_rewriting) {
            with_rewriting++;
        } else {
            without_rewriting++;
        }
    }
    
    std::cout << "\nTest Case Summary:\n";
    std::cout << "  Test cases with expected rewritings: " << with_rewriting << "\n";
    std::cout << "  Test cases without rewritings: " << without_rewriting << "\n";
    
    // Display sample test cases
    std::cout << "\n=== Sample Test Cases ===\n\n";
    
    for (int i = 0; i < 3 && i < testcases.size(); ++i) {
        const auto& tc = testcases[i];
        std::cout << "Test Case " << tc.id << ": " << tc.description << "\n";
        std::cout << "Query: " << tc.query << "\n";
        std::cout << "Views: " << tc.views.size() << "\n";
        std::cout << "Expected: " << (tc.should_have_rewriting ? "REWRITING EXISTS" : "NO REWRITING") << "\n";
        std::cout << std::string(60, '-') << "\n\n";
    }
    
    return 0;
}
