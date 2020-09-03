#!/usr/bin/env Rscript

library(tidyverse)
library(maditr)
library(reshape)
library(ggplot2)

# https://ggplot2.tidyverse.org/reference/geom_ribbon.html

# https://www.r-graph-gallery.com/104-plot-lines-with-error-envelopes-ggplot2.html

# aes(ymin = mean - se, ymax = mean + se)

# https://pastebin.com/FQFbUEcd

# https://stackoverflow.com/questions/38470111/how-to-graph-with-geom-ribbon

counters <- list("ac_state_ST_INFECTED", "ac_infected_state_ST_MILD", "ac_infected_state_ST_SEVERE", "ac_infected_state_ST_CRITICAL", "ac_state_ST_DEAD", "ac_state_ST_HEALTHY", "r", "reproductive", "g_reported", "ac_total_infected_state_ST_CRITICAL", "ac_total_infected_state_ST_SEVERE", "ac_reported")

colors <- c("purple", "yellow", "orange", "red", "blue", "green", "cyan", "cyan", "gray")

args = commandArgs(trailingOnly=TRUE)

if (length(args) != 3)
	stop("Usage: plot-cycles.R <mean.csv> <stddeev.csv> <output_base>\n")

csv_mean = args[1]
csv_error = args[2]
output_base = args[3]

data_mean_ = read.csv(csv_mean, header = TRUE)
data_error_ = read.csv(csv_error, header = TRUE)

i <- 1
for (counter in counters) {
	print(counter)

	data_mean <- data_mean_
	data_error <- data_error_

	data <- data_mean %>% select(cycle)
	data$mean <- data_mean[, counter]
	data$error <- data_error[, counter]

	data$lower_bound <- data$mean - data$error
	data$upper_bound <- data$mean + data$error

	#print(data)
	#stop()

	# h <- ggplot(huron, aes(year))

	# h + geom_ribbon(aes(ymin = level - 1, ymax = level + 1), fill = "grey70") + geom_line(aes(y = level))

	#png(file=paste0(output_base, "-", counter, ".png"))

	png(filename = paste0(output_base, "-", counter, ".png"),
	    width = 1300, height = 700, units = "px", pointsize = 12,
	     bg = "white",  res = NA)

#	pdf(file=paste0(output_base, "-", counter, ".pdf"))

	p <- ggplot(data, aes(x =cycle)) +
	geom_ribbon(aes(ymin=lower_bound, ymax=upper_bound), 
	                 #alpha=0.1,       #transparency
	                 linetype=1,      #solid, dashed or other line types
	                 colour="grey70", #border line color
	                 size=1,          #border line size
	                 fill=colors[i])    #fill color
	
	p <- p + theme(axis.text=element_text(size=30), axis.title=element_text(size=30,face="bold"))

	p <- p + scale_x_continuous(breaks = round(seq(0, max(data$cycle), by = 30),1))

	if (counter == "reproductive") {
		p <- p + scale_y_continuous(breaks = round(seq(0, max(data$upper_bound), by = 0.5),1))
		p <- p + geom_hline(yintercept=1, color = "black")
		p <- p + geom_hline(yintercept=2, color = "black")
		p <- p + geom_hline(yintercept=3, color = "black")
	}

	print(p)
	
	i <- i + 1
#p <- ggplot(data, aes(x = Q, y = C, color=Name, group=Name))

# p <- p + geom_line()
# p + geom_ribbon(aes(ymin=Lower_Bound, ymax=Upper_Bound), 
#                 alpha=0.1,       #transparency
#                 linetype=1,      #solid, dashed or other line types
#                 colour="grey70", #border line color
#                 size=1,          #border line size
#                 fill="green")    #fill color
}
