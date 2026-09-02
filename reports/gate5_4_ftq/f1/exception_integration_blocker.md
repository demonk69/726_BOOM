# Exception Integration Blocker

F1 verifies only the standalone FTQ operation that retains a validated owner
through capture and kills younger entries. The current product's terminal
exception flow does not provide complete owner validation, EPC/cause capture
ordering, reference invalidation, and trap-fetch generation ownership needed
for integration. Therefore `READY_FOR_PRODUCT_FTQ_INTEGRATION=false` and the
next stage must be the read-only PF0 product integration/exception review.
