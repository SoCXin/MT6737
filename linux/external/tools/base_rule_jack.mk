#add .jar dependency to .jack to build .jar for custom release
define base-rules-release-jack-hook
$(eval \
ifeq ($(LOCAL_IS_STATIC_JAVA_LIBRARY),true)
ifdef LOCAL_JACK_ENABLED
$(full_classes_jack) : $(common_javalib.jar)
endif
endif
)
endef
