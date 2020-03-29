#!/usr/bin/env Rscript

data = read.csv("results-cycles.csv", header = TRUE)

pdf(file=paste0("results-cycles.pdf"), width=11)

myylim = 100000

labels = c("sim-infected", "sim-suscetive", "sir-infected", "sir-suscetive")

plot(data$cycle, data$infected, ylim=c(0,myylim), type="o", col="orange")
lines(data$cycle, data$ac_healthy, ylim=c(0,myylim), type="o", col="green")
lines(data$cycle, data$sir_i, ylim=c(0,myylim), type="o", col="gray")
lines(data$cycle, data$sir_s, ylim=c(0,myylim), type="o", col="gray")

grid(col = "gray", lwd=2)

legend(150, 100000, labels, cex=0.8, col=c("orange","green","gray","gray"), pch=21:22, lty=1:2);
