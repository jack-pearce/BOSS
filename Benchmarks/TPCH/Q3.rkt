(Top
 (Group
  (Project
   (Join
    (Select
     'CUSTOMER
     (Where
      (Equal 'C_MKTSEGMENT "BUILDING"))
     )
    (Join
     (Select
      'ORDERS
      (Where
       (Greater
        (Date "1995-03-15")
        (Date 'O_ORDERDATE))
       )
      )
     (Select
      'LINEITEM
      (Where
       (Greater
        (Date 'L_SHIPDATE)
        (Date "1995-03-22")
        )
       )
      )
     (Where (Equal 'L_ORDERKEY 'O_ORDERKEY))
     
     )
    (Where (Equal 'C_CUSTKEY 'O_CUSTKEY))
    )
   (As 'Expr1009 (Multiply 'L_EXTENDEDPRICE  (Subtract 1 'L_DISCOUNT))
       'L_ORDERKEY 'L_ORDERKEY
       'O_ORDERDATE 'O_ORDERDATE
       'O_SHIPPRIORITY 'O_SHIPPRIORITY
       )
   )
  (By 'L_ORDERKEY 'O_ORDERDATE 'O_SHIPPRIORITY)
  (Sum 'Expr1009)
  )
 (By 'Expr1009)
 10
 )
