#!/usr/bin/env Rscript

data = read.csv("results-cycles.csv", header = TRUE)

pdf(file=paste0("results-cycles.pdf"), width=11)

plot(data$cycle, data$infected, type="o", col="orange")
