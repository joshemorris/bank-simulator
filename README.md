Bank Simulator
------------------------
This lab simulates a small bank with 4 customers and 3 atms.
The program begins by reading cusdtomers account number and 
balances from corresponding files. It then tracks atm transactions
in threads for each atm and updates customer acounts using main
after three transactions have taken place. Main records each transaction
to the customer's account files 

Implemented using pthreads

compile
-----------------
`make`

run
-----------------
'./bank'

input
-----------------
Customer files

ATM transaction files

output
-----------------
customer files

customer final balances

