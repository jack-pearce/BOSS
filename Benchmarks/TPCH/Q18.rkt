(Top
 (Group
  (Join
   (Join
    (Join
     (Project
      (Select
       (Group
        'LINEITEM
        (By 'L_ORDERKEY)
        (Sum 'L_QUANTITY)
        )
       (Where (Greater 'L_QUANTITY 300))
       )
      (As 'INNER_L_ORDERKEY 'L_ORDERKEY)
      )
     'LINEITEM
     (Where (Equals 'L_ORDERKEY 'INNER_L_ORDERKEY))
     )
    'ORDERS
    (Where (Equals 'O_ORDERKEY 'L_ORDERKEY))
    )
   'CUSTOMER
   (Where (Equals 'C_CUSTKEY 'O_CUSTKEY))
   )
  (By 'C_NAME 'C_CUSTKEY 'O_ORDERKEY 'O_ORDERDATE 'O_TOTALPRICE)
  (Sum 'L_QUANTITY)
  )
 (By 'O_TOTALPRICE 'O_ORDERDATE)
 100
 )
