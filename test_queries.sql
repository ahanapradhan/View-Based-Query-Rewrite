-- Q1:
Select c_name, s_name, n_name from customer, supplier, nation where c_nationkey = s_nationkey and s_nationkey = n_nationkey;

-- V1:
Select c_name, c_nationkey, s_name, s_nationkey from customer, supplier where c_nationkey = s_nationkey;
-- V2:
Select c_name, c_nationkey, n_name, n_nationkey from customer, nation where c_nationkey = n_nationkey;
-- V3
Select c_name, c_nationkey from customer;

