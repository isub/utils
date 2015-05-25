#ORACLE_HOME=/u01/app/oracle/product/11.2.0/db_1
OCI_L=-L$(ORACLE_HOME)/lib/
LIB=-lpthread -lclntsh
OCI_H=-I$(ORACLE_HOME)/rdbms/public
UTL_DIR=../../utils
UTL_H=-I../../

all: clean dbselector

dbselector: ./obj/dbselector.o ./obj/log.o ./obj/config.o ./obj/dbpool.o
	g++ ./obj/dbselector.o ./obj/log.o ./obj/config.o ./obj/dbpool.o -o ../dbselector $(OCI_L) $(LIB)
./obj/dbselector.o: main.cpp
	g++ -c main.cpp $(OCI_H) $(UTL_H) -o ./obj/dbselector.o
./obj/log.o: $(UTL_DIR)/log/log.cpp $(UTL_DIR)/log/log.h
	g++ -c $(UTL_DIR)/log/log.cpp -o ./obj/log.o
./obj/config.o: $(UTL_DIR)/config/config.cpp $(UTL_DIR)/config/config.h
	g++ -c $(UTL_DIR)/config/config.cpp -o ./obj/config.o
./obj/dbpool.o: $(UTL_DIR)/dbpool/dbpool.cpp $(UTL_DIR)/dbpool/dbpool.h
	g++ -c $(UTL_DIR)/dbpool/dbpool.cpp $(OCI_H) $(UTL_H) -o ./obj/dbpool.o


install:
	cp -f ../dbselector /usr/local/bin/dbselector

clean:
	rm -f ./obj/*.o ../dbselector