(Order
 (Group
  (Project
   (Join
     'NATION
     (Join
      (Join
       'SUPPLIER
       (Join
        (Join
         (Select 'PART
                 (Where (StringContains 'P_NAME "green")))
         'PARTSUPP
         (Where (Equal ps_partkey p_partkey))
         )
        'LINEITEM
        (Where (And (Equal 'PS_PARTKEY 'L_PARTKEY)
                    (Equal 'PS_SUPPKEY 'L_SUPPKEY)))
        )
       (Where (And
               (Equal 'S_SUPPKEY 'L_SUPPKEY)
               (Equal 'S_SUPPKEY 'L_SUPPKEY)
               ))
       )
      'ORDERS
      (Where (Equal 'O_ORDERKEY 'L_ORDERKEY))
      )
     (Where (Equal 'S_NATIONKEY 'N_NATIONKEY))
     )
   (As 'nation 'N_NAME
       'o_year (Year 'O_ORDERDATE)
       'amount
       (Subtract (Multiply 'L_EXTENDEDPRICE (Subtract 1 l_discount))
                 (Multiply 'PS_SUPPLYCOST 'L_QUANTITY))
       )
   )
  (By 'nation 'o_year)
  (Sum 'amount)
  )
 (By 'nation 'o_year)
 )
