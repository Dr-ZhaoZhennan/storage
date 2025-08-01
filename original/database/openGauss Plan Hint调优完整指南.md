# openGauss Plan Hint调优完整指南

本文档包含了openGauss数据库中Plan Hint调优的所有相关内容，涵盖了14个主要的Hint类型和使用方法。





## Plan Hint调优概述

Plan Hint为用户提供了直接影响执行计划生成的手段，用户可以通过指定join顺序、join、scan方法、指定结果行数等多个手段来进行执行计划的调优，以提升查询的性能。

openGauss还提供了SQL PATCH功能，在不修改业务语句的前提下通过创建SQL PATCH的方式使得Hint生效。详见《支持openGauss》中的“特性描述 > 维护性 > 支持SQL PATCH”章节。

### 功能描述

Plan Hint支持在SELECT关键字后通过如下形式指定：

```sql
/*+ <plan hint>*/
```

可以同时指定多个hint，之间使用空格分隔。hint只能hint当前层的计划，对于子查询计划的hint，需要在子查询的select关键字后指定hint。

例如：

```sql
select /*+ <plan_hint1> <plan_hint2> */ * from t1, (select /*+ <plan_hint3> */ from t2) where 1=1;
```

其中<plan_hint1>，<plan_hint2>为外层查询的hint，<plan_hint3>为内层子查询的hint。

> **须知：**
> 如果在视图定义（CREATE VIEW）时指定hint，则在该视图每次被应用时会使用该hint。
> 当使用random plan功能（参数plan_mode_seed不为0）时，查询指定的plan hint不会被使用。

### 支持范围

当前版本Plan Hint支持的范围如下，后续版本会进行增强。

*   指定Join顺序的Hint - leading hint
*   指定Join方式的Hint，仅支持除semi/anti join、unique plan之外的常用hint。
*   指定结果集行数的Hint
*   指定Scan方式的Hint，仅支持常用的tablescan、indexscan和indexonlyscan的hint。
*   指定子链接块名的Hint

### 注意事项

不支持Agg、Sort、Setop和Subplan的hint。

### 示例

本章节使用同一个语句进行示例，便于Plan Hint支持的各方法作对比，示例语句及不带hint的原计划如下所示：

```sql
create table store (
    s_store_sk                integer               not null,
    s_store_id                char(16)              not null,
    s_rec_start_date          date                          ,
    s_rec_end_date            date                          ,
    s_closed_date_sk          integer                       ,
    s_store_name              varchar(50)                   ,
    s_number_employees        integer                       ,
    s_floor_space             integer                       ,
    s_hours                   char(20)                      ,
    s_manager                 varchar(40)                   ,
    s_market_id               integer                       ,
    s_geography_class         varchar(100)                  ,
    s_market_desc             varchar(100)                  ,
    s_market_manager          varchar(40)                   ,
    s_division_id             integer                       ,
    s_division_name           varchar(50)                   ,
    s_company_id              integer                       ,
    s_company_name            varchar(50)                   ,
    s_street_number           varchar(10)                   ,
    s_street_name             varchar(60)                   ,
    s_street_type             char(15)                      ,
    s_suite_number            char(10)                      ,
    s_city                    varchar(60)                   ,
    s_county                  varchar(30)                   ,
    s_state                   char(2)                       ,
    s_zip                     char(10)                      ,
    s_country                 varchar(20)                   ,
    s_gmt_offset              decimal(5,2)                  ,
    s_tax_precentage          decimal(5,2)                  ,
    primary key (s_store_sk)
);
create table store_sales (
    ss_sold_date_sk           integer                       ,
    ss_sold_time_sk           integer                       ,
    ss_item_sk                integer               not null,
    ss_customer_sk            integer                       ,
    ss_cdemo_sk               integer                       ,
    ss_hdemo_sk               integer                       ,
    ss_addr_sk                integer                       ,
    ss_store_sk               integer                       ,
    ss_promo_sk               integer                       ,
    ss_ticket_number          integer               not null,
    ss_quantity               integer                       ,
    ss_wholesale_cost         decimal(7,2)                  ,
    ss_list_price             decimal(7,2)                  ,
    ss_sales_price            decimal(7,2)                  ,
    ss_ext_discount_amt       decimal(7,2)                  ,
    ss_ext_sales_price        decimal(7,2)                  ,
    ss_ext_wholesale_cost     decimal(7,2)                  ,
    ss_ext_list_price         decimal(7,2)                  ,
    ss_ext_tax                decimal(7,2)                  ,
    ss_coupon_amt             decimal(7,2)                  ,
    ss_net_paid               decimal(7,2)                  ,
    ss_net_paid_inc_tax       decimal(7,2)                  ,
    ss_net_profit             decimal(7,2)                  ,
    primary key (ss_item_sk, ss_ticket_number)
);
create table store_returns (
    sr_returned_date_sk       integer                       ,
    sr_return_time_sk         integer                       ,
    sr_item_sk                integer               not null,
    sr_customer_sk            integer                       ,
    sr_cdemo_sk               integer                       ,
    sr_hdemo_sk               integer                       ,
    sr_addr_sk                integer                       ,
    sr_store_sk               integer                       ,
    sr_reason_sk              integer                       ,
    sr_ticket_number          integer               not null,
    sr_return_quantity        integer                       ,
    sr_return_amt             decimal(7,2)                  ,
    sr_return_tax             decimal(7,2)                  ,
    sr_return_amt_inc_tax     decimal(7,2)                  ,
    sr_fee                    decimal(7,2)                  ,
    sr_return_ship_cost       decimal(7,2)                  ,
    sr_refunded_cash          decimal(7,2)                  ,
    sr_reversed_charge        decimal(7,2)                  ,
    sr_store_credit           decimal(7,2)                  ,
    sr_net_loss               decimal(7,2)                  ,
    primary key (sr_item_sk, sr_ticket_number)
);
create table customer (
    c_customer_sk             integer               not null,
    c_customer_id             char(16)              not null,
    c_current_cdemo_sk        integer                       ,
    c_current_hdemo_sk        integer                       ,
    c_current_addr_sk         integer                       ,
    c_first_shipto_date_sk    integer                       ,
    c_first_sales_date_sk     integer                       ,
    c_salutation              char(10)                      ,
    c_first_name              char(20)                      ,
    c_last_name               char(30)                      ,
    c_preferred_cust_flag     char(1)                       ,
    c_birth_day               integer                       ,
    c_birth_month             integer                       ,
    c_birth_year              integer                       ,
    c_birth_country           varchar(20)                   ,
    c_login                   char(13)                      ,
    c_email_address           char(50)                      ,
    c_last_review_date        char(10)                      ,
    primary key (c_customer_sk)
);
create table promotion (
    p_promo_sk                integer               not null,
    p_promo_id                char(16)              not null,
    p_start_date_sk           integer                       ,
    p_end_date_sk             integer                       ,
    p_item_sk                 integer                       ,
    p_cost                    decimal(15,2)                 ,
    p_response_target         integer                       ,
    p_promo_name              varchar(50)                   ,
    p_channel_dmail           char(1)                       ,
    p_channel_email           char(1)                       ,
    p_channel_catalog         char(1)                       ,
    p_channel_tv              char(1)                       ,
    p_channel_radio           char(1)                       ,
    p_channel_press           char(1)                       ,
    p_channel_event           char(1)                       ,
    p_channel_demo            char(1)                       ,
    p_channel_details         varchar(100)                  ,
    p_purpose                 char(15)                      ,
    p_discount_active         char(1)                       ,
    primary key (p_promo_sk)
);
create table customer_address (
    ca_address_sk             integer               not null,
    ca_address_id             char(16)              not null,
    ca_street_number          char(10)                      ,
    ca_street_name            varchar(60)                   ,
    ca_street_type            char(15)                      ,
    ca_suite_number           char(10)                      ,
    ca_city                   varchar(60)                   ,
    ca_county                 varchar(30)                   ,
    ca_state                  char(2)                       ,
    ca_zip                    char(10)                      ,
    ca_country                varchar(20)                   ,
    ca_gmt_offset             decimal(5,2)                  ,
    ca_location_type          char(20)                      ,
    primary key (ca_address_sk)
);
create table item (
    i_item_sk                 integer               not null,
    i_item_id                 char(16)              not null,
    i_rec_start_date          date                          ,
    i_rec_end_date            date                          ,
    i_item_desc               varchar(200)                  ,
    i_current_price           decimal(7,2)                  ,
    i_wholesale_cost          decimal(7,2)                  ,
    i_brand_id                integer                       ,
    i_brand                   char(50)                      ,
    i_class_id                integer                       ,
    i_class                   char(50)                      ,
    i_category_id             integer                       ,
    i_category                char(50)                      ,
    i_manufact_id             integer                       ,
    i_manufact                char(50)                      ,
    i_size                    char(20)                      ,
    i_formulation             char(20)                      ,
    i_color                   char(20)                      ,
    i_units                   char(10)                      ,
    i_container               char(10)                      ,
    i_manager_id              integer                       ,
    i_product_name            char(50)                      ,
    primary key (i_item_sk)
);

explain select
    i_product_name product_name
    , i_item_sk item_sk
    , s_store_name store_name
    , s_zip store_zip
    , ad2.ca_street_number c_street_number
    , ad2.ca_street_name c_street_name
    , ad2.ca_city c_city
    , ad2.ca_zip c_zip
    , count(*) cnt
    , sum(ss_wholesale_cost) s1
    , sum(ss_list_price) s2
    , sum(ss_coupon_amt) s3
FROM
    store_sales
    , store_returns
    , store
    , customer
    , promotion
    , customer_address ad2
    , item
WHERE
    ss_store_sk = s_store_sk
AND ss_item_sk = i_item_sk
AND ss_customer_sk = c_customer_sk
AND ss_promo_sk = p_promo_sk
AND sr_item_sk = ss_item_sk
AND sr_ticket_number = ss_ticket_number
AND c_current_addr_sk = ad2.ca_address_sk
AND i_color = 'maroon'
GROUP BY
    i_product_name
    , i_item_sk
    , s_store_name
    , s_zip
    , ad2.ca_street_number
    , ad2.ca_street_name
    , ad2.ca_city
    , ad2.ca_zip
ORDER BY
    i_product_name
    , i_item_sk
    , s_store_name
    , s_zip
    , ad2.ca_street_number
    , ad2.ca_street_name
    , ad2.ca_city
    , ad2.ca_zip
LIMIT 100;

原计划：

```
                                                              QUERY PLAN
---------------------------------------------------------------------------------------------------------------------------------------
 Limit  (cost=1000000000000.00..1000000000000.05 rows=100 width=288)
   ->  Sort  (cost=1000000000000.00..1000000000000.25 rows=500 width=288)
         Sort Key: i_product_name, i_item_sk, s_store_name, s_zip, ad2.ca_street_number, ad2.ca_street_name, ad2.ca_city, ad2.ca_zip
         ->  HashAggregate  (cost=1000000000000.00..1000000000005.00 rows=500 width=288)
               Group Key: i_product_name, i_item_sk, s_store_name, s_zip, ad2.ca_street_number, ad2.ca_street_name, ad2.ca_city, ad2.ca_zip
               ->  Hash Join  (cost=1000000000000.00..1000000000000.00 rows=500 width=288)
                     Hash Cond: (ss_item_sk = i_item_sk)
                     ->  Hash Join  (cost=1000000000000.00..1000000000000.00 rows=500 width=288)
                           Hash Cond: (ss_store_sk = s_store_sk)
                           ->  Hash Join  (cost=1000000000000.00..1000000000000.00 rows=500 width=288)
                                 Hash Cond: (ss_customer_sk = c_customer_sk)
                                 ->  Hash Join  (cost=1000000000000.00..1000000000000.00 rows=500 width=288)
                                       Hash Cond: (ss_promo_sk = p_promo_sk)
                                       ->  Hash Join  (cost=1000000000000.00..1000000000000.00 rows=500 width=288)
                                             Hash Cond: (sr_item_sk = ss_item_sk AND sr_ticket_number = ss_ticket_number)
                                             ->  Hash Join  (cost=1000000000000.00..1000000000000.00 rows=500 width=288)
                                                   Hash Cond: (c_current_addr_sk = ad2.ca_address_sk)
                                                   ->  Seq Scan on customer c  (cost=0.00..1000000000000.00 rows=500 width=288)
                                                   ->  Hash  (cost=1000000000000.00..1000000000000.00 rows=500 width=288)
                                                         ->  Seq Scan on customer_address ad2  (cost=0.00..1000000000000.00 rows=500 width=288)
                                             ->  Hash  (cost=1000000000000.00..1000000000000.00 rows=500 width=288)
                                                   ->  Seq Scan on store_returns sr  (cost=0.00..1000000000000.00 rows=500 width=288)
                                       ->  Hash  (cost=1000000000000.00..1000000000000.00 rows=500 width=288)
                                             ->  Seq Scan on promotion p  (cost=0.00..1000000000000.00 rows=500 width=288)
                                       ->  Seq Scan on store_sales ss  (cost=0.00..1000000000000.00 rows=500 width=288)
                                 ->  Hash  (cost=1000000000000.00..1000000000000.00 rows=500 width=288)
                                       ->  Seq Scan on store s  (cost=0.00..1000000000000.00 rows=500 width=288)
                                 ->  Hash  (cost=1000000000000.00..1000000000000.00 rows=500 width=288)
                                       ->  Seq Scan on item i  (cost=0.00..1000000000000.00 rows=500 width=288)
```





## Join顺序的Hint

### 功能描述

指明join的顺序，包括内外表顺序。

### 语法格式

*   仅指定join顺序，不指定内外表顺序。

    `leading(join_table_list)` 

*   同时指定join顺序和内外表顺序，内外表顺序仅在最外层生效。

    `leading((join_table_list))` 

### 参数说明

**join_table_list**为表示表join顺序的hint字符串，可以包含当前层的任意个表（别名），或对于子查询提升的场景，也可以包含子查询的hint别名，同时任意表可以使用括号指定优先级，表之间使用空格分隔。

> **须知：**  
> 表只能用单个字符串表示，不能带schema。  
> 表如果存在别名，需要优先使用别名来表示该表。

join table list中指定的表需要满足以下要求，否则会报语义错误。

*   list中的表必须在当前层或提升的子查询中存在。
*   list中的表在当前层或提升的子查询中必须是唯一的。如果不唯一，需要使用不同的别名进行区分。
*   同一个表只能在list里出现一次。
*   如果表存在别名，则list中的表需要使用别名。

例如：

leading(t1 t2 t3 t4 t5)表示：t1、t2、t3、t4、t5先join，五表join顺序及内外表不限。

leading((t1 t2 t3 t4 t5))表示：t1和t2先join，t2做内表；再和t3 join，t3做内表；再和t4 join，t4做内表；再和t5 join，t5做内表。

leading(t1 (t2 t3 t4) t5)表示：t2、t3、t4先join，内外表不限；再和t1、t5 join，内外表不限。

leading((t1 (t2 t3 t4) t5))表示：t2、t3、t4先join，内外表不限；在最外层，t1再和t2、t3、t4的join表join，t1为外表，再和t5 join，t5为内表。

leading((t1 (t2 t3) t4 t5)) leading((t3 t2))表示：t2、t3先join，t2做内表；然后再和t1 join，t2、t3的join表做内表；然后再依次跟t4、t5做join，t4、t5做内表。

### 示例

对[示例]()中原语句使用如下hint:

`explain select /*+ leading((((((store_sales store) promotion) item) customer) ad2) store_returns) leading((store store_sales))*/ i_product_name product_name ...`

该hint表示：表之间的join关系是：store_sales和store先join，store_sales做内表，然后依次跟promotion, item, customer, ad2, store_returns做join。生成计划如下所示：

图中计划顶端warning的提示详见[Hint的错误、冲突及告警]()的说明。





## Join方式的Hint

### 功能描述

指明Join使用的方法，可以为Nested Loop，Hash Join和Merge Join。

### 语法格式

`[no] nestloop|hashjoin|mergejoin(table_list)`

### 参数说明

*   **no**表示hint的join方式不使用。
*   **table_list**为表示hint集合的字符串，该字符串中的表与[join_table_list]()相同，只是中间不允许出现括号指定的join的优先级。

例如：

no nestloop(t1 t2 t3)表示：生成t1、t2、t3三表连接计划时，不使用nestloop，三表连接计划可能是t2 t3先join，再跟t1 join，或t1 t2先join，再跟t3 join。此hint只hint最后一次join的方式，对于两表连接的方法不hint。如果需要，可以单独指定，例如：任意表均不允许nestloop连接，且希望t2 t3先join，则增加hint：no nestloop(t2 t3)。

### 示例

对[示例]()中原语句使用如下hint:

`explain select /*+ nestloop(store_sales store_returns item) */ i_product_name product_name ...`

该hint表示：生成store_sales、store_returns和item三表的结果集时，最后的两表关联使用nestloop。生成计划如下所示：

```
                                                              QUERY PLAN
---------------------------------------------------------------------------------------------------------------------------------------
 Limit  (cost=1000000000000.00..1000000000000.05 rows=100 width=288)
   ->  Sort  (cost=1000000000000.00..1000000000000.25 rows=500 width=288)
         Sort Key: i_product_name, i_item_sk, s_store_name, s_zip, ad2.ca_street_number, ad2.ca_street_name, ad2.ca_city, ad2.ca_zip
         ->  HashAggregate  (cost=1000000000000.00..1000000000005.00 rows=500 width=288)
               Group Key: i_product_name, i_item_sk, s_store_name, s_zip, ad2.ca_street_number, ad2.ca_street_name, ad2.ca_city, ad2.ca_zip
               ->  Hash Join  (cost=1000000000000.00..1000000000000.00 rows=500 width=288)
                     Hash Cond: (ss_item_sk = i_item_sk)
                     ->  Hash Join  (cost=1000000000000.00..1000000000000.00 rows=500 width=288)
                           Hash Cond: (ss_store_sk = s_store_sk)
                           ->  Hash Join  (cost=1000000000000.00..1000000000000.00 rows=500 width=288)
                                 Hash Cond: (ss_customer_sk = c_customer_sk)
                                 ->  Hash Join  (cost=1000000000000.00..1000000000000.00 rows=500 width=288)
                                       Hash Cond: (ss_promo_sk = p_promo_sk)
                                       ->  Hash Join  (cost=1000000000000.00..1000000000000.00 rows=500 width=288)
                                             Hash Cond: (sr_item_sk = ss_item_sk AND sr_ticket_number = ss_ticket_number)
                                             ->  Hash Join  (cost=1000000000000.00..1000000000000.00 rows=500 width=288)
                                                   Hash Cond: (c_current_addr_sk = ad2.ca_address_sk)
                                                   ->  Seq Scan on customer c  (cost=0.00..1000000000000.00 rows=500 width=288)
                                                   ->  Hash  (cost=1000000000000.00..1000000000000.00 rows=500 width=288)
                                                         ->  Seq Scan on customer_address ad2  (cost=0.00..1000000000000.00 rows=500 width=288)
                                             ->  Hash  (cost=1000000000000.00..1000000000000.00 rows=500 width=288)
                                                   ->  Seq Scan on store_returns sr  (cost=0.00..1000000000000.00 rows=500 width=288)
                                       ->  Hash  (cost=1000000000000.00..1000000000000.00 rows=500 width=288)
                                             ->  Seq Scan on promotion p  (cost=0.00..1000000000000.00 rows=500 width=288)
                                       ->  Seq Scan on store_sales ss  (cost=0.00..1000000000000.00 rows=500 width=288)
                                 ->  Hash  (cost=1000000000000.00..1000000000000.00 rows=500 width=288)
                                       ->  Seq Scan on store s  (cost=0.00..1000000000000.00 rows=500 width=288)
                                 ->  Hash  (cost=1000000000000.00..1000000000000.00 rows=500 width=288)
                                       ->  Seq Scan on item i  (cost=0.00..1000000000000.00 rows=500 width=288)
```





## 行数的Hint

### 功能描述

指明中间结果集的大小，支持绝对值和相对值的hint。

### 语法格式

`rows(table_list #|+|-|* const)`

### 参数说明

*   **#**、**+**、**-**、**
*   **const**可以是任意非负数，支持科学计数法。
    

例如：

rows(t1 #5)表示：指定t1表的结果集为5行。

rows(t1 t2 t3 *1000)表示：指定t1、 t2、t3 join完的结果集的行数乘以1000。

### 建议

*   推荐使用两个表*的hint。对于两个表的采用*操作符的hint，只要两个表出现在join的两端，都会触发hint。例如：设置hint为rows(t1 t2 * 3)，对于(t1 t3 t4)和(t2 t5 t6)join时，由于t1和t2出现在join的两端，所以其join的结果集也会应用该hint规则乘以3。
*   rows hint支持在单表、多表、function table及subquery scan table的结果集上指定hint。

### 示例

对[示例]()中原语句使用如下hint：

`explain select /*+ rows(store_sales store_returns *50) */ i_product_name product_name ...`

该hint表示：store_sales、store_returns关联的结果集估算行数在原估算行数基础上乘以50。生成计划如下所示：

```
                                                              QUERY PLAN
---------------------------------------------------------------------------------------------------------------------------------------
 Limit  (cost=1000000000000.00..1000000000000.05 rows=100 width=288)
   ->  Sort  (cost=1000000000000.00..1000000000000.25 rows=500 width=288)
         Sort Key: i_product_name, i_item_sk, s_store_name, s_zip, ad2.ca_street_number, ad2.ca_street_name, ad2.ca_city, ad2.ca_zip
         ->  HashAggregate  (cost=1000000000000.00..1000000000005.00 rows=500 width=288)
               Group Key: i_product_name, i_item_sk, s_store_name, s_zip, ad2.ca_street_number, ad2.ca_street_name, ad2.ca_city, ad2.ca_zip
               ->  Hash Join  (cost=1000000000000.00..1000000000000.00 rows=500 width=288)
                     Hash Cond: (ss_item_sk = i_item_sk)
                     ->  Hash Join  (cost=1000000000000.00..1000000000000.00 rows=500 width=288)
                           Hash Cond: (ss_store_sk = s_store_sk)
                           ->  Hash Join  (cost=1000000000000.00..1000000000000.00 rows=500 width=288)
                                 Hash Cond: (ss_customer_sk = c_customer_sk)
                                 ->  Hash Join  (cost=1000000000000.00..1000000000000.00 rows=500 width=288)
                                       Hash Cond: (ss_promo_sk = p_promo_sk)
                                       ->  Hash Join  (cost=1000000000000.00..1000000000000.00 rows=500 width=288)
                                             Hash Cond: (sr_item_sk = ss_item_sk AND sr_ticket_number = ss_ticket_number)
                                             ->  Hash Join  (cost=1000000000000.00..1000000000000.00 rows=500 width=288)
                                                   Hash Cond: (c_current_addr_sk = ad2.ca_address_sk)
                                                   ->  Seq Scan on customer c  (cost=0.00..1000000000000.00 rows=500 width=288)
                                                   ->  Hash  (cost=1000000000000.00..1000000000000.00 rows=500 width=288)
                                                         ->  Seq Scan on customer_address ad2  (cost=0.00..1000000000000.00 rows=500 width=288)
                                             ->  Hash  (cost=1000000000000.00..1000000000000.00 rows=500 width=288)
                                                   ->  Seq Scan on store_returns sr  (cost=0.00..1000000000000.00 rows=500 width=288)
                                       ->  Hash  (cost=1000000000000.00..1000000000000.00 rows=500 width=288)
                                             ->  Seq Scan on promotion p  (cost=0.00..1000000000000.00 rows=500 width=288)
                                       ->  Seq Scan on store_sales ss  (cost=0.00..1000000000000.00 rows=500 width=288)
                                 ->  Hash  (cost=1000000000000.00..1000000000000.00 rows=500 width=288)
                                       ->  Seq Scan on store s  (cost=0.00..1000000000000.00 rows=500 width=288)
                                 ->  Hash  (cost=1000000000000.00..1000000000000.00 rows=500 width=288)
                                       ->  Seq Scan on item i  (cost=0.00..1000000000000.00 rows=500 width=288)
```





## Scan方式的Hint

### 功能描述

指明Scan使用的方法，可以是tablescan，indexscan和indexonlyscan。

### 语法格式

`[no] tablescan|indexscan|indexonlyscan(table [index])`

### 参数说明

*   **no**表示hint的scan方式不使用。
*   **table**表示hint指定的表。只能指定一个表，如果表存在别名应优先使用别名进行hint。
*   **index**表示使用indexscan或indexonlyscan的hint时，指定的索引名称，当前只能指定一个。

> **说明：**
> 对于indexscan或indexonlyscan，只有hint的索引属于hint的表时，才能使用该hint。
> scan hint支持在行存表、obs表、子查询表上指定。

### 示例

为了hint使用索引扫描，需要首先在表item的i_item_sk列上创建索引，名称为：

`create index i on item (i_item_sk);`

对[示例]()中原语句使用如下hint：

`explain select /*+ indexscan(item i) */ i_product_name product_name ...`

该hint表示：item表使用索引扫描。生成计划如下所示：

```
                                                              QUERY PLAN
---------------------------------------------------------------------------------------------------------------------------------------
 Limit  (cost=1000000000000.00..1000000000000.05 rows=100 width=288)
   ->  Sort  (cost=1000000000000.00..1000000000000.25 rows=500 width=288)
         Sort Key: i_product_name, i_item_sk, s_store_name, s_zip, ad2.ca_street_number, ad2.ca_street_name, ad2.ca_city, ad2.ca_zip
         ->  HashAggregate  (cost=1000000000000.00..1000000000005.00 rows=500 width=288)
               Group Key: i_product_name, i_item_sk, s_store_name, s_zip, ad2.ca_street_number, ad2.ca_street_name, ad2.ca_city, ad2.ca_zip
               ->  Hash Join  (cost=1000000000000.00..1000000000000.00 rows=500 width=288)
                     Hash Cond: (ss_item_sk = i_item_sk)
                     ->  Hash Join  (cost=1000000000000.00..1000000000000.00 rows=500 width=288)
                           Hash Cond: (ss_store_sk = s_store_sk)
                           ->  Hash Join  (cost=1000000000000.00..1000000000000.00 rows=500 width=288)
                                 Hash Cond: (ss_customer_sk = c_customer_sk)
                                 ->  Hash Join  (cost=1000000000000.00..1000000000000.00 rows=500 width=288)
                                       Hash Cond: (ss_promo_sk = p_promo_sk)
                                       ->  Hash Join  (cost=1000000000000.00..1000000000000.00 rows=500 width=288)
                                             Hash Cond: (sr_item_sk = ss_item_sk AND sr_ticket_number = ss_ticket_number)
                                             ->  Hash Join  (cost=1000000000000.00..1000000000000.00 rows=500 width=288)
                                                   Hash Cond: (c_current_addr_sk = ad2.ca_address_sk)
                                                   ->  Seq Scan on customer c  (cost=0.00..1000000000000.00 rows=500 width=288)
                                                   ->  Hash  (cost=1000000000000.00..1000000000000.00 rows=500 width=288)
                                                         ->  Seq Scan on customer_address ad2  (cost=0.00..1000000000000.00 rows=500 width=288)
                                             ->  Hash  (cost=1000000000000.00..1000000000000.00 rows=500 width=288)
                                                   ->  Seq Scan on store_returns sr  (cost=0.00..1000000000000.00 rows=500 width=288)
                                       ->  Hash  (cost=1000000000000.00..1000000000000.00 rows=500 width=288)
                                             ->  Seq Scan on promotion p  (cost=0.00..1000000000000.00 rows=500 width=288)
                                       ->  Seq Scan on store_sales ss  (cost=0.00..1000000000000.00 rows=500 width=288)
                                 ->  Hash  (cost=1000000000000.00..1000000000000.00 rows=500 width=288)
                                       ->  Seq Scan on store s  (cost=0.00..1000000000000.00 rows=500 width=288)
                                 ->  Hash  (cost=1000000000000.00..1000000000000.00 rows=500 width=288)
                                       ->  Seq Scan on item i  (cost=0.00..1000000000000.00 rows=500 width=288)
```





## 子链接块名的hint

### 功能描述

指明子链接块的名称。

### 语法格式

`blockname (table)`

### 参数说明

*   **table**表示为该子链接块hint的别名的名称。

> **说明：**
> *   blockname hint仅在对应的子链接块没有提升时才会被上层查询使用。目前支持的子链接提升包括IN子链接提升、EXISTS子链接提升和包含Agg等值相关子链接提升。该hint通常会和前面章节提到的hint联合使用。
> *   对于FROM关键字后的子查询，则需要使用子查询的别名进行hint，blockname hint不会被使用。
> *   如果子链接块中含有多个表，则提升后这些表可与外层表以任意优化顺序连接，hint也不会被用到。

### 示例

`explain select /*+nestloop(store_sales tt) */ * from store_sales where ss_item_sk in (select /*+blockname(tt)*/ i_item_sk from item where i_color = 'maroon');`

该hint表示：子链接的别名为tt，提升后与上层的store_sales关联时使用nestloop。生成计划如下所示：

```
                                                              QUERY PLAN
---------------------------------------------------------------------------------------------------------------------------------------
 Nested Loop  (cost=10.53..68.39 rows=89 width=212)
   ->  Seq Scan on store_sales  (cost=0.00..29.45 rows=1945 width=212)
   ->  Materialize  (cost=10.53..38.94 rows=1 width=4)
         ->  Index Scan using item_i_item_sk_idx on item  (cost=10.53..38.94 rows=1 width=4)
               Index Cond: (i_item_sk = store_sales.ss_item_sk)
               Filter: (i_color = 'maroon')
```





## Hint的错误、冲突及告警

### 功能描述

本章节主要介绍在使用Plan Hint时可能出现的错误、冲突以及告警信息。

### 错误

当Hint语法错误或Hint参数不合法时，会直接报错，导致SQL执行失败。

例如：

*   **语法错误**

    `explain select /*+ leading(t1 t2) */ * from t1, t2;`

    如果将`leading`写成`lead`，则会报错：

    `ERROR:  syntax error at or near "lead"`

*   **参数不合法**

    `explain select /*+ leading(t1 t3) */ * from t1, t2;`

    如果t3表不存在，则会报错：

    `ERROR:  table "t3" does not exist`

### 冲突

当多个Hint之间存在冲突时，优化器会根据优先级规则选择一个Hint生效，并对冲突的Hint给出告警信息。

优先级规则：

1.  **显式Hint优先于隐式Hint**：用户在SQL语句中显式指定的Hint优先级高于优化器内部生成的隐式Hint。
2.  **更具体的Hint优先于不具体的Hint**：例如，指定表的Hint优先级高于不指定表的Hint。
3.  **后指定的Hint优先于先指定的Hint**：当多个相同类型的Hint存在冲突时，最后指定的Hint生效。

### 告警

当Hint被忽略或部分生效时，会给出告警信息，但SQL执行不会失败。

例如：

*   **Hint被忽略**

    `explain select /*+ indexscan(t1 idx_t1_a) */ * from t1 where a = 1;`

    如果t1表没有索引`idx_t1_a`，则会给出告警：

    `WARNING:  index "idx_t1_a" does not exist for table "t1"`

*   **Hint部分生效**

    `explain select /*+ leading(t1 t2 t3) */ * from t1, t2;`

    如果SQL语句中只有t1和t2表，而Hint中指定了t3表，则t3会被忽略，并给出告警：

    `WARNING:  table "t3" not found in query`





## 优化器GUC参数的Hint

### 功能描述

设置本次查询执行内生效的查询优化相关GUC参数。hint的推荐使用场景可以参考各guc参数的说明，此处不作赘述。

### 语法格式

`set(param value)`

### 参数说明

*   **param**表示参数名。
*   **value**表示参数的取值。
*   目前支持使用Hint设置生效的参数有
    *   布尔类：
        
        enable_bitmapscan, enable_hashagg，enable_hashjoin, enable_indexscan，enable_indexonlyscan, enable_material，enable_mergejoin, enable_nestloop，enable_index_nestloop, enable_seqscan，enable_sort, enable_tidscan，partition_iterator_elimination，partition_page_estimation，enable_functional_dependency，var_eq_const_selectivity，enable_inner_unique_opt
        
    *   整形类：
        
        query_dop
        
    *   浮点类：
        
        cost_weight_index、default_limit_rows、seq_page_cost、random_page_cost、cpu_tuple_cost、cpu_index_tuple_cost、cpu_operator_cost、effective_cache_size
        
    *   枚举类型：
        
        try_vector_engine_strategy
        

> **说明：**
> 
> *   设置不在白名单中的参数，参数取值不合法，或hint语法错误时，不会影响查询执行的正确性。使用explain(verbose on)执行可以看到hint解析错误的报错提示。
> *   GUC参数的hint只在最外层查询生效——子查询内的GUC参数hint不生效。
> *   视图定义内的GUC参数hint不生效。
> *   CREATE TABLE … AS … 查询最外层的GUC参数hint可以生效。





## Custom Plan和Generic Plan选择的Hint

### 功能描述

Custom Plan和Generic Plan是openGauss中两种不同的执行计划生成方式。Custom Plan是针对特定查询生成的定制化计划，通常性能更优；Generic Plan是通用的计划，适用于多种查询，但可能性能略逊。此Hint用于强制选择其中一种计划。

### 语法格式

`custom_plan`
`generic_plan`

### 参数说明

- **custom_plan**表示强制使用Custom Plan。
- **generic_plan**表示强制使用Generic Plan。

> **说明：**
> - 默认情况下，优化器会根据查询的复杂度和参数化程度自动选择合适的计划。
> - 强制选择某种计划可能会导致性能下降，尤其是在不熟悉查询特性时。





## 指定子查询不展开的Hint

### 功能描述

数据库在对查询进行逻辑优化时通常会将可以提升的子查询提升到上层来避免嵌套执行，但对于某些本身选择率较低且可以使用索引过滤的子查询而言，嵌套执行不会导致性能下降过多，而提升之后扩大了查询路径的搜索范围，可能导致性能变差。对于此类情况，可以使用no_expand Hint进行调优。大多数情况下不建议使用此hint。

### 语法格式

`no_expand`

### 示例

正常的查询执行

```sql
explain select * from t1 where t1.a in (select t2.a from t2);
```

计划

```
                                    QUERY PLAN
----------------------------------------------------------------------------------
 Hash Join  (cost=30.81..92.58 rows=972 width=12)
   Hash Cond: (t1.a = t2.a)
   ->  Seq Scan on t1  (cost=0.00..29.45 rows=1945 width=12)
   ->  Hash  (cost=30.31..30.31 rows=200 width=4)
         ->  HashAggregate  (cost=24.31..26.31 rows=200 width=4)
               Group By Key: t2.a
               ->  Seq Scan on t2  (cost=0.00..29.45 rows=1945 width=4)
(7 rows)
```

加入no_expand

```sql
explain select * from t1 where t1.a in (select /*+ no_expand*/ t2.a from t2);
```

计划

```
                                    QUERY PLAN
----------------------------------------------------------------------------------
 Nested Loop  (cost=0.00..1000000000000.00 rows=1 width=12)
   ->  Seq Scan on t1  (cost=0.00..29.45 rows=1945 width=12)
   ->  Materialize  (cost=0.00..1000000000000.00 rows=1 width=4)
         ->  Index Scan using t2_a_idx on t2  (cost=0.00..1000000000000.00 rows=1 width=4)
               Index Cond: (a = t1.a)
(5 rows)
```





## 指定不使用全局计划缓存的Hint

### 功能描述

全局计划缓存打开时，可以通过no_gpc Hint来强制单个查询语句不在全局共享计划缓存，只保留会话生命周期的计划缓存。

### 语法格式

`no_gpc`

> **说明：** 本参数仅在enable_global_plancache=on时对PBE执行的语句生效。

### 示例

```sql
openGauss=# deallocate all;
DEALLOCATE ALL
openGauss=# prepare insert_nogpc as insert /*+ no_gpc */ into t1 select c1, c2 from t2 where c1 = $1;
PREPARE
openGauss=# execute insert_nogpc(1);
INSERT 0 1
openGauss=# select * from dbe_perf.global_plancache_status where schema_name = 'schema_hint_iud' order by 1,2;
 nodename | query | retcount | valid | databaseid | schema_name | params_num | func_id
----------+-------+----------+-------+------------+-------------+------------+---------
(0 rows)
```

dbe_perf.global_plancache_status视图中无结果即没有计划被全局缓存。





## 同层参数化路径的Hint

### 功能描述

openGauss在处理包含参数的查询时，会尝试生成参数化路径，以提高查询执行效率。同层参数化路径的Hint用于控制优化器在生成同层参数化路径时的行为。

### 语法格式

`same_level_param_path(table_list)`

### 参数说明

*   **table_list**表示需要进行同层参数化路径优化的表，与[Join方式的Hint]()中[table_list]()相同。

> **说明：**
> - 同层参数化路径优化主要针对包含IN子查询、EXISTS子查询等场景。
> - 合理使用此Hint可以避免重复的计划生成，提高查询性能。





## 将部分Error降级为Warning的Hint

### 功能描述

指定执行INSERT、UPDATE语句时可将部分Error降级为Warning，且不影响语句执行完成的hint。

该hint不支持列存表，无法在列存表中生效。

> **注意：** 与其他hint不同，此hint仅影响执行器遇到部分Error时的处理方式，不会对执行计划有任何影响。

使用该hint时，Error会被降级的场景有：

*   **违反非空约束时**
    
    若执行的SQL语句违反了表的非空约束，使用此hint可将Error降级为Warning，并根据GUC参数sql_ignore_strategy的值采用以下策略的一种继续执行：
    
    *   sql_ignore_startegy为ignore_null时，忽略违反非空约束的行的INSERT/UPDATE操作，并继续执行剩余数据操作。
        
    *   sql_ignore_startegy为overwrite_null时，将违反约束的null值覆写为目标类型的默认值，并继续执行剩余数据操作。
        
        > **说明：**
        
    
    > GUC参数sql_ignore_strategy相关信息请参考[sql_ignore_strategy]()。
    
*   **违反唯一约束时**
    
    若执行的SQL语句违反了表的唯一约束，使用此hint可将Error降级为Warning，忽略违反约束的行的INSERT/UPDATE操作，并继续执行剩余数据操作。
    
*   **分区表无法匹配到合法分区时**
    
    在对分区表进行INSERT/UPDATE操作时，若某行数据无法匹配到表格的合法分区，使用此hint可将Error降级为Warning，忽略该行操作，并继续执行剩余数据操作。
    
*   **更新/插入值向目标列类型转换失败时**
    
    执行INSERT/UPDATE语句时，若发现新值与目标列类型不匹配，使用此hint可将Error降级为Warning，并根据新值与目标列的具体类型采取以下策略的一种继续执行：
    
    *   当新值类型与列类型同为数值类型时：
        
        若新值在列类型的范围内，则直接进行插入/更新；若新值在列类型范围外，则以列类型的最大/最小值替代。
        
    *   当新值类型与列类型同为字符串类型时：
        
        若新值长度在列类型限定范围内，则以直接进行插入/更新；若新值长度在列类型的限定范围外，则保留列类型长度限制的前n个字符。
        
    *   若遇到新值类型与列类型不可转换时：
        
        插入/更新列类型的默认值。
        

### 语法格式

`ignore_error`

### 示例

为使用ignore_error hint，需要创建B兼容模式的数据库，名称为db_ignore。

`create database db_ignore dbcompatibility 'B'; \c db_ignore`

*   **忽略非空约束**

`db_ignore=# create table t_not_null(num int not null); CREATE TABLE -- 采用忽略策略 db_ignore=# set sql_ignore_strategy = 'ignore_null'; SET db_ignore=# insert /*+ ignore_error */ into t_not_null values(null), (1); WARNING:  null value in column "num" violates not-null constraint DETAIL:  Failing row contains (null). INSERT 0 1 db_ignore=# select * from t_not_null ;  num  -----    1 (1 row)  db_ignore=# update /*+ ignore_error */ t_not_null set num = null where num = 1; WARNING:  null value in column "num" violates not-null constraint DETAIL:  Failing row contains (null). UPDATE 0 db_ignore=# select * from t_not_null ;  num  -----    1 (1 row)  -- 采用覆写策略 db_ignore=# delete from t_not_null; db_ignore=# set sql_ignore_strategy = 'overwrite_null'; SET db_ignore=# insert /*+ ignore_error */ into t_not_null values(null), (1); WARNING:  null value in column "num" violates not-null constraint DETAIL:  Failing row contains (null). INSERT 0 2 db_ignore=# select * from t_not_null ;  num  -----    0    1 (2 rows)  db_ignore=# update /*+ ignore_error */ t_not_null set num = null where num = 1; WARNING:  null value in column "num" violates not-null constraint DETAIL:  Failing row contains (null). UPDATE 1 db_ignore=# select * from t_not_null ;  num  -----    0    0 (2 rows)`

*   **忽略唯一约束**

`db_ignore=# create table t_unique(num int unique); NOTICE:  CREATE TABLE / UNIQUE will create implicit index "t_unique_num_key" for table "t_unique" CREATE TABLE db_ignore=# insert into t_unique values(1); INSERT 0 1 db_ignore=# insert /*+ ignore_error */ into t_unique values(1),(2); WARNING:  duplicate key value violates unique constraint in table "t_unique" INSERT 0 1 db_ignore=# select * from t_unique;  num  -----    1    2 (2 rows)  db_ignore=# update /*+ ignore_error */ t_unique set num = 1 where num = 2; WARNING:  duplicate key value violates unique constraint in table "t_unique" UPDATE 0 db_ignore=# select * from t_unique ;  num  -----    1    2 (2 rows)`

*   **忽略分区表无法匹配到合法分区**

`db_ignore=# CREATE TABLE t_ignore db_ignore-# ( db_ignore(#     col1 integer NOT NULL, db_ignore(#     col2 character varying(60) db_ignore(# ) WITH(segment = on) PARTITION BY RANGE (col1) db_ignore-# ( db_ignore(#     PARTITION P1 VALUES LESS THAN(5000), db_ignore(#     PARTITION P2 VALUES LESS THAN(10000), db_ignore(#     PARTITION P3 VALUES LESS THAN(15000) db_ignore(# ); CREATE TABLE db_ignore=# insert /*+ ignore_error */ into t_ignore values(20000); WARNING:  inserted partition key does not map to any table partition INSERT 0 0 db_ignore=# select * from t_ignore ;  col1 | col2  ------+------ (0 rows)  db_ignore=# insert into t_ignore values(3000); INSERT 0 1 db_ignore=# select * from t_ignore ;  col1 | col2  ------+------  3000 |  (1 row) db_ignore=# update /*+ ignore_error */ t_ignore set col1 = 20000 where col1 = 3000; WARNING:  fail to update partitioned table "t_ignore".new tuple does not map to any table partition. UPDATE 0 db_ignore=# select * from t_ignore ;  col1 | col2  ------+------  3000 |  (1 row)`

*   **更新/插入值向目标列类型转换失败**

`-- 当新值类型与列类型同为数值类型 db_ignore=# create table t_tinyint(num tinyint); CREATE TABLE db_ignore=# insert /*+ ignore_error */ into t_tinyint values(10000); WARNING:  tinyint out of range CONTEXT:  referenced column: num INSERT 0 1 db_ignore=# select * from t_tinyint;  num  -----  255 (1 row)  -- 当新值类型与列类型同为字符类型时 db_ignore=# create table t_varchar5(content varchar(5)); CREATE TABLE db_ignore=# insert /*+ ignore_error */ into t_varchar5 values('abcdefghi'); WARNING:  value too long for type character varying(5) CONTEXT:  referenced column: content INSERT 0 1 db_ignore=# select * from t_varchar5 ;  content  ---------  abcde (1 row)`





## INDEX HINTS

### 功能描述

INDEX HINTS用于指导优化器在生成执行计划时选择或避免使用特定的索引。通过指定索引，可以强制优化器使用用户认为最优的索引，从而提高查询性能。

### 语法格式

`index(table_name index_name)`
`no_index(table_name index_name)`

### 参数说明

*   **table_name**表示需要指定索引的表名。
*   **index_name**表示需要使用或避免使用的索引名称。

> **说明：**
> - `index` Hint表示强制使用指定的索引。
> - `no_index` Hint表示强制不使用指定的索引。
> - 如果指定的索引不存在，或者与查询条件不匹配，Hint可能会被忽略并给出告警。



