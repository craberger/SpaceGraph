#!/bin/bash

echo "Nuking existing data"
${NEO4J_HOME}/bin/neo4j stop
old_pwd=`pwd`
cd ${NEO4J_HOME} && rm -rf data/*
cd $old_pwd
${NEO4J_HOME}/bin/neo4j start

echo "Loading data"
${NEO4J_HOME}/bin/neo4j-shell << EOF
CREATE CONSTRAINT ON (node:AwesomeNode) ASSERT node.node_id IS UNIQUE;
USING PERIODIC COMMIT LOAD CSV FROM 'file://$1/glab_undirected/data.txt' AS line FIELDTERMINATOR ' ' MERGE (fromNode:AwesomeNode { node_id: toInt(line[0]) }) MERGE (toNode:AwesomeNode { node_id: toInt(line[1]) }) CREATE (fromNode)-[:AwesomeEdge]->(toNode);
EOF

echo "Triangle counting"
${NEO4J_HOME}/bin/neo4j-shell << EOF
MATCH (n:AwesomeNode)-[:AwesomeEdge*2]->(n3:AwesomeNode) WHERE n3<-[:AwesomeEdge]-n RETURN COUNT(n);
EOF

echo "Clique counting"
${NEO4J_HOME}/bin/neo4j-shell << EOF
MATCH (n:AwesomeNode)-->(n2:AwesomeNode)-->(n3:AwesomeNode)-->(n4:AwesomeNode) WHERE  n-->n3 AND  n2-->n4 AND n-->n4 RETURN COUNT(n);
EOF

echo "Cycle counting"
${NEO4J_HOME}/bin/neo4j-shell << EOF
MATCH (n:AwesomeNode)-[:AwesomeEdge*3]->(n3:AwesomeNode) WHERE n3<-[:AwesomeEdge]-(n) RETURN COUNT(n);
EOF