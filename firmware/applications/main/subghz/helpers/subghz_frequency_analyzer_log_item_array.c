#include "subghz_frequency_analyzer_log_item_array.h"

const char*
    subghz_frequency_analyzer_log_get_order_name(SubGhzFrequencyAnalyzerLogOrderBy order_by) {
    if(order_by == SubGhzFrequencyAnalyzerLogOrderBySeqAsc) {
        return "Пор. А";
    }
    if(order_by == SubGhzFrequencyAnalyzerLogOrderByCountDesc) {
        return "Кол. Д";
    }
    if(order_by == SubGhzFrequencyAnalyzerLogOrderByCountAsc) {
        return "Кол. А";
    }
    if(order_by == SubGhzFrequencyAnalyzerLogOrderByRSSIDesc) {
        return "RSSI D";
    }
    if(order_by == SubGhzFrequencyAnalyzerLogOrderByRSSIAsc) {
        return "RSSI A";
    }
    if(order_by == SubGhzFrequencyAnalyzerLogOrderByFrequencyDesc) {
        return "Част. Д";
    }
    if(order_by == SubGhzFrequencyAnalyzerLogOrderByFrequencyAsc) {
        return "Част. А";
    }
    return "Пор. Д";
}
