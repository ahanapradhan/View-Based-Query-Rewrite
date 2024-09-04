-- Q1:
Select c_name, s_name, n_name from customer, supplier, nation where c_nationkey = s_nationkey and s_nationkey = n_nationkey;

-- V1:
Create view V1 as
Select c_name, c_nationkey, s_name, s_nationkey from customer, supplier where c_nationkey = s_nationkey;
-- V2:
Create view V2 as
Select c_name, c_nationkey, n_name from customer, nation where c_nationkey = n_nationkey;
-- V3
Create view V3 as
Select c_name, c_nationkey from customer;


-- Q2
select c_name, n_name, s_name from customer, nation, supplier where c_nationkey = n_nationkey and n_nationkey = s_nationkey and c_acctbal < 1000;

-- V1
Create view V1 as
Select c_name, c_nationkey from customer where c_acctbal < 1000;
-- V2
Create view V2 as
select c_name, c_nationkey, s_name, s_nationkey from customer, supplier where c_nationkey = s_nationkey and c_acctbal < 2500;
-- V3
Create view V3 as
select c_name, c_nationkey, n_name, n_nationkey from customer, nation where c_nationkey = n_nationkey and c_acctbal > 2500;


-- Q3
select c_name, n_name, s_name from customer, nation, supplier where c_nationkey = n_nationkey and n_nationkey = s_nationkey and c_acctbal < 1000;

-- V1
Create view V1 as
Select c_name, c_nationkey from customer where c_acctbal < 1000;
-- V2
Create view V2 as
select c_name, c_nationkey, s_name, s_nationkey from customer LEFT OUTER JOIN supplier ON c_nationkey = s_nationkey and c_acctbal < 2500;
-- V3
Create view V3 as
select c_name, c_nationkey, n_name, n_nationkey from customer FULL OUTER JOIN nation ON c_nationkey = n_nationkey and c_acctbal > 2500;


-- Q4
select c_name, n_name, s_name from customer, nation, supplier where c_nationkey = n_nationkey and n_nationkey = s_nationkey and c_acctbal < 5000 and s_acctbal > 9999;

-- V1
Create view V1 as
select c_name, c_nationkey, s_name, s_nationkey, n_name from customer, supplier, nation where c_nationkey = s_nationkey and s_nationkey = n_nationkey and c_acctbal between 4001 and 4500 and s_acctbal > 10000;
-- V2
Create view V2 as
select c_name, c_nationkey, s_name, n_nationkey, n_name from customer, supplier, nation where c_nationkey = s_nationkey and s_nationkey = n_nationkey and c_acctbal between 0 and 4000 and s_acctbal between 8000 and 100001;
-- V3
Create view V3 as
select c_name, c_nationkey, s_name, n_name from customer, supplier, nation where c_nationkey = s_nationkey and c_nationkey = n_nationkey and c_acctbal between 4501 and 7500 and s_acctbal between 5000 and 20000;



