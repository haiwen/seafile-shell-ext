VERSION ?= 0.1.0
ARCHIVE_NAME = seafile-shell-ext-$(VERSION)

dist:
	git archive -v --prefix=${ARCHIVE_NAME}/ HEAD | gzip > $(ARCHIVE_NAME).tar.gz
