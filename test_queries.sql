-- Q1:
Select c_name, s_name, n_name from customer, supplier, nation where c_nationkey = s_nationkey and s_nationkey = n_nationkey;

-- V1:
Select c_name, c_nationkey, s_name, s_nationkey from customer, supplier where c_nationkey = s_nationkey;
-- V2:
Select c_name, c_nationkey, n_name from customer, nation where c_nationkey = n_nationkey;
-- V3
Select c_name, c_nationkey from customer;


-- Q2
select c_name, n_name, s_name from customer, nation, supplier where c_nationkey = n_nationkey and n_nationkey = s_nationkey and c_acctbal < 1000;

-- V1
Select c_name, c_nationkey from customer where c_acctbal < 1000;
-- V2
select c_name, c_nationkey, s_name, s_nationkey from customer, supplier where c_nationkey = s_nationkey and c_acctbal < 2500;
-- V3
select c_name, c_nationkey, n_name, n_nationkey from customer, nation where c_nationkey = n_nationkey and c_acctbal > 2500;


-- Q3
select c_name, n_name, s_name from customer, nation, supplier where c_nationkey = n_nationkey and n_nationkey = s_nationkey and c_acctbal < 1000;

-- V1
Select c_name, c_nationkey from customer where c_acctbal < 1000;
-- V2
select c_name, c_nationkey, s_name, s_nationkey from customer INNER JOIN supplier ON c_nationkey = s_nationkey and c_acctbal < 2500;
-- V3
select c_name, c_nationkey, n_name, n_nationkey from customer INNER JOIN nation ON c_nationkey = n_nationkey and c_acctbal > 2500;



