DROP TABLE edges;
DROP TABLE uedges;
CREATE TABLE edges (source INT NOT NULL, dest INT NOT NULL) SEGMENTED BY HASH(source, dest) ALL NODES;
CREATE TABLE uedges (source INT NOT NULL, dest INT NOT NULL) SEGMENTED BY HASH(source, dest) ALL NODES;

copy edges from :file direct delimiter ' ';

INSERT INTO uedges(source, dest) (
   SELECT source, dest FROM edges
);

INSERT INTO uedges(source, dest) (
   SELECT dest, source FROM edges
);

\timing
-- Cycle counting
SELECT COUNT(*) FROM uedges;
SELECT COUNT(*) FROM edges;
SELECT COUNT(*) FROM
   uedges e1 JOIN uedges e2 ON e1.dest = e2.source JOIN uedges e3 ON e2.dest = e3.source JOIN uedges e4 ON e3.source = e4.dest AND e4.source = e1.source AND e1.source < e2.dest AND e2.source < e3.dest;
\timing
