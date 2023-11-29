  'unprivileged' in schema.properties[property]) {\x0a          shouldCheck = true;\x0a          break;\x0a        }\x0a      }\x0a      return shouldCheck;\x0a    }\x0a    var checkUnprivileged = shouldCheckUnprivileged();\x0a\x0a    // TODO(kalman/cduvall): Make GetAvailability handle this, then delete the\x0a    // supporting code.\x0a    if (!isSchemaNodeSupported(schema, platform, manifestVersion)) {\x0a      console.error('chrome.' + schema.namespace + ' is not supported on ' +\x0a                    'this platform or manifest version');\x0a      return undefined;\x0a    }\x0a\x0a    var mod = {};\x0a\x0a    var namespaces = $String.split(schema.namespace, '.');\x0a    for (var index = 0, name; name = namespaces[index]; index++) {\x0a      mod[name] = mod[name] || {};\x0a      mod = mod[name];\x0a    }\x0a\x0a    if (schema.types) {\x0a      $Array.forEach(schema.types, function(t) {\x0a        if (!isSchemaNodeSupported(t, platform, manifestVersion))\x0a          return;\x0a\x0a        // Add types to global schemaValidator; the types we depend on from\x0a        // other namespaces will be added as needed.\x0a        schemaUtils.schemaValidator.addTypes(t);\x0a\x0a        // Generate symbols for enums.\x0a        var enumValues = t['enum'];\x0a        if (enumValues) {\x0a          // Type IDs are qualified with the namespace during compilation,\x0a          // unfortunately, so remove it here.\x0a          logging.DCHECK($String.substr(t.id, 0, schema.namespace.length) ==\x0a                             schema.namespace);\x0a          // Note: + 1 because it ends in a '.', e.g., 'fooApi.Type'.\x0a          var id = $String.substr(t.id, schema.namespace.length + 1);\x0a          mod[id] = {};\x0a          $Array.forEach(enumValues, function(enumValue) {\x0a            // Note: enums can be declared either as a list of strings\x0a            // ['foo', 'bar'] or as a list of objects\x0a            // [{'name': 'foo'}, {'name': 'bar'}].\x0a            enumValue = $Object.hasOwnProperty(enumValue, 'name') ?\x0a                enumValue.name : enumValue;\x0a            if (enumValue) {  // Avoid setting any empty enums.\x0a              // Make all properties in ALL_CAPS_STYLE.\x0a              //\x0a              // The built-in versions of $String.replace call other built-ins,\x0a              // which may be clobbered. Instead, manually build the property\x0a              // name.\x0a              //\x0a              // If the first character is a digit (we know it must be one of\x0a              // a digit, a letter, or an underscore), precede it with an\x0a              // underscore.\x0a              var propertyName = ($RegExp.exec(/\d/, enumValue[0])) ? '_' : '';\x0a              for (var i = 0; i < enumValue.length; ++i) {\x0a                var next;\x0a                if (i > 0 && $RegExp.exec(/[a-z]/, enumValue[i-1]) &&\x0a                    $RegExp.exec(/[A-Z]/, enumValue[i])) {\x0a                  // Replace myEnum-Foo with my_Enum-Foo:\x0a                  next = '_' + enumValue[i];\x0a                } else if ($RegExp.exec(/\W/, enumValue[i])) {\x0a                  // Replace my_Enum-Foo with my_Enum_Foo:\x0a                  next = '_';\x0a                } else {\x0a                  next = enumValue[i];\x0a                }\x0a                propertyName += next;\x0a              }\x0a              // Uppercase (replace my_Enum_Foo with MY_ENUM_FOO):\x0a              propertyName = $String.toUpperCase(propertyName);\x0a              mod[id][propertyName] = enumValue;\x0a            }\x0a          });\x0a        }\x0a      }, this);\x0a    }\x0a\x0a    // TODO(cduvall): Take out when all APIs have been converted to features.\x0a    // Returns whether access to the content of a schema should be denied,\x0a    // based on the presence of "unprivileged" and whether this is an\x0a    // extension process (versus e.g. a content script).\x0a    function isSchemaAccessAllowed(itemSchema) {\x0a      return (contextType == 'BLESSED_EXTENSION') ||\x0a             schema.unprivileged ||\x0a             itemSchema.unprivileged;\x0a    };\x0a\x0a    // Setup Functions.\x0a    if (schema.functions) {\x0a      $Array.forEach(schema.functions, function(functionDef) {\x0a        if (functionDef.name in mod) {\x0a          throw new Error('Function ' + functionDef.name +\x0a                          ' already defined in ' + schema.namespace);\x0a        }\x0a\x0a        if (!isSchemaNodeSupported(functionDef, platform, manifestVersion)) {\x0a          this.apiFunctions_.registerUnavailable(functionDef.name);\x0a          return;\x0a        }\x0a\x0a        var apiFunction = { __proto__: null };\x0a        apiFunction.definition = functionDef;\x0a        apiFunction.name = schema.namespace + '.' + functionDef.name;\x0a\x0a        if (!GetAvailability(apiFunction.name).is_available ||\x0a            (checkUnprivileged && !isSchemaAccessAllowed(functionDef))) {\x0a          this.apiFunctions_.registerUnavailable(functionDef.name);\x0a          return;\x0a        }\x0a\x0a        // TODO(aa): It would be best to run this in a unit test, but in order\x0a        // to do that we would need to better factor this code so that it\x0a        // doesn't depend on so much v8::Extension machinery.\x0a        if (logging.DCHECK_IS_ON() &&\x0a            schemaUtils.isFunctionSignatureAmbiguous(apiFunction.definition)) {\x0a          throw new Error(\x0a              apiFunction.name + ' has ambiguous optional arguments. ' +\x0a              'To implement custom disambiguation logic, add ' +\x0a              '"allowAmbiguousOptionalArguments" to the function\'s schema.');\x0a        }\x0a\x0a        this.apiFunctions_.register(functionDef.name, apiFunction);\x0a\x0a        mod[functionDef.name] = $Function.bind(function() {\x0a          var args = $Array.slice(arguments);\x0a          if (this.updateArgumentsPreValidate)\x0a            args = $Function.apply(this.updateArgumentsPreValidate, this, args);\x0a\x0a          args = schemaUtils.normalizeArgumentsAndValidate(args, this);\x0a          if (this.updateArgumentsPostValidate) {\x0a            args = $Function.apply(this.updateArgumentsPostValidate,\x0a                                   this,\x0a                                   args);\x0a          }\x0a\x0a          sendRequestHandler.clearCalledSendRequest();\x0a\x0a          var retval;\x0a          if (this.handleRequest) {\x0a            retval = $Function.apply(this.handleRequest, this, args);\x0a          } else {\x0a            var optArgs = {\x0a              __proto__: null,\x0a              customCallback: this.customCallback\x0a            };\x0a            retval = sendRequest(this.name, args,\x0a                                 this.definition.parameters,\x0a                                 optArgs);\x0a          }\x0a          sendRequestHandler.clearCalledSendRequest();\x0a\x0a          // Validate return value if in sanity check mode.\x0a          if (logging.DCHECK_IS_ON() && this.definition.returns)\x0a            schemaUtils.validate([retval], [this.definition.returns]);\x0a          return retval;\x0a        }, apiFunction);\x0a      }, this);\x0a    }\x0a\x0a    // Setup Events\x0a    if (schema.events) {\x0a      $Array.forEach(schema.events, function(eventDef) {\x0a        if (eventDef.name in mod) {\x0a          throw new Error('Event ' + eventDef.name +\x0a                          ' already defined in ' + schema.namespace);\x0a        }\x0a        if (!isSchemaNodeSupported(eventDef, platform, manifestVersion))\x0a          return;\x0a\x0a        var eventName = schema.namespace + "." + eventDef.name;\x0a        if (!GetAvailability(eventName).is_available ||\x0a            (checkUnprivileged && !isSchemaAccessAllowed(eventDef))) {\x0a          return;\x0a        }\x0a\x0a        var options = eventDef.options || {};\x0a        if (eventDef.filters && eventDef.filters.length > 0)\x0a          options.supportsFilters = true;\x0a\x0a        var parameters = eventDef.parameters;\x0a        if (this.customEvent_) {\x0a          mod[eventDef.name] = new this.customEvent_(\x0a              eventName, parameters, eventDef.extraParameters, options);\x0a        } else {\x0a          mod[eventDef.name] = new Event(eventName, parameters, options);\x0a        }\x0a      }, this);\x0a    }\x0a\x0a    function addProperties(m, parentDef) {\x0a      var properties = parentDef.properties;\x0a      if (!properties)\x0a        return;\x0a\x0a      forEach(properties, function(propertyName, propertyDef) {\x0a        if (propertyName in m)\x0a          return;  // TODO(kalman): be strict like functions/events somehow.\x0a        if (!isSchemaNodeSupported(propertyDef, platform, manifestVersion))\x0a          return;\x0a        if (!GetAvailability(schema.namespace + "." +\x0a              propertyName).is_available ||\x0a            (checkUnprivileged && !isSchemaAccessAllowed(propertyDef))) {\x0a          return;\x0a        }\x0a\x0a        // |value| is eventually added to |m|, the exposed API. Make copies\x0a        // of everything from the schema. (The schema is also frozen, so as long\x0a        // as we don't make any modifications, shallow copies are fine.)\x0a        var value;\x0a        if ($Array.isArray(propertyDef.value))\x0a          value = $Array.slice(propertyDef.value);\x0a        else if (typeof propertyDef.value === 'object')\x0a          value = $Object.assign({}, propertyDef.value);\x0a        else\x0a          value = p...

 Line:256
 query_str is:d543a38b0365f3544c9dd97d0009201a1e4d51e052274fbcdb0c7d0271896e71
The query d543a38b0365f3544c9dd97d0009201a1e4d51e052274fbcdb0c7d0271896e71 is indeed a member!
Key is 0x1a979a705879 <String[4]: name> Source code is:function generate() {\x0a    // NB: It's important to load the schema during generation rather than\x0a    // setting it beforehand so that we're more confident the schema we're\x0a    // loading is real, and not one that was injected by a page intercepting\x0a    // Binding.generate.\x0a    // Additionally, since the schema is an object returned from a native\x0a    // handler, its properties don't have the custom getters/setters that a page\x0a    // may have put on Object.prototype, and the object is frozen by v8.\x0a    var schema = schemaRegistry.GetSchema(this.apiName_);\x0a\x0a    function shouldCheckUnprivileged() {\x0a      var shouldCheck = 'unprivileged' in schema;\x0a      if (shouldCheck)\x0a        return shouldCheck;\x0a\x0a      $Array.forEach(['functions', 'events'], function(type) {\x0a        if ($Object.hasOwnProperty(schema, type)) {\x0a          $Array.forEach(schema[type], function(node) {\x0a            if ('unprivileged' in node)\x0a              shouldCheck = true;\x0a          });\x0a        }\x0a      });\x0a      if (shouldCheck)\x0a        return shouldCheck;\x0a\x0a      for (var property in schema.properties) {\x0a        if ($Object.hasOwnProperty(schema, property) &&\x0a            'unprivileged' in schema.properties[property]) {\x0a          shouldCheck = true;\x0a          break;\x0a        }\x0a      }\x0a      return shouldCheck;\x0a    }\x0a    var checkUnprivileged = shouldCheckUnprivileged();\x0a\x0a    // TODO(kalman/cduvall): Make GetAvailability handle this, then delete the\x0a    // supporting code.\x0a    if (!isSchemaNodeSupported(schema, platform, manifestVersion)) {\x0a      console.error('chrome.' + schema.namespace + ' is not supported on ' +\x0a                    'this platform or manifest version');\x0a      return undefined;\x0a    }\x0a\x0a    var mod = {};\x0a\x0a    var namespaces = $String.split(schema.namespace, '.');\x0a    for (var index = 0, name; name = namespaces[index]; index++) {\x0a      mod[name] = mod[name] || {};\x0a      mod = mod[name];\x0a    }\x0a\x0a    if (schema.types) {\x0a      $Array.forEach(schema.types, function(t) {\x0a        if (!isSchemaNodeSupported(t, platform, manifestVersion))\x0a          return;\x0a\x0a        // Add types to global schemaValidator; the types we depend on from\x0a        // other namespaces will be added as needed.\x0a        schemaUtils.schemaValidator.addTypes(t);\x0a\x0a        // Generate symbols for enums.\x0a        var enumValues = t['enum'];\x0a        if (enumValues) {\x0a          // Type IDs are qualified with the namespace during compilation,\x0a          // unfortunately, so remove it here.\x0a          logging.DCHECK($String.substr(t.id, 0, schema.namespace.length) ==\x0a                             schema.namespace);\x0a          // Note: + 1 because it ends in a '.', e.g., 'fooApi.Type'.\x0a          var id = $String.substr(t.id, schema.namespace.length + 1);\x0a          mod[id] = {};\x0a          $Array.forEach(enumValues, function(enumValue) {\x0a            // Note: enums can be declared either as a list of strings\x0a            // ['foo', 'bar'] or as a list of objects\x0a            // [{'name': 'foo'}, {'name': 'bar'}].\x0a            enumValue = $Object.hasOwnProperty(enumValue, 'name') ?\x0a                enumValue.name : enumValue;\x0a            if (enumValue) {  // Avoid setting any empty enums.\x0a              // Make all properties in ALL_CAPS_STYLE.\x0a              //\x0a              // The built-in versions of $String.replace call other built-ins,\x0a              // which may be clobbered. Instead, manually build the property\x0a              // name.\x0a              //\x0a              // If the first character is a digit (we know it must be one of\x0a              // a digit, a letter, or an underscore), precede it with an\x0a              // underscore.\x0a              var propertyName = ($RegExp.exec(/\d/, enumValue[0])) ? '_' : '';\x0a              for (var i = 0; i < enumValue.length; ++i) {\x0a                var next;\x0a                if (i > 0 && $RegExp.exec(/[a-z]/, enumValue[i-1]) &&\x0a                    $RegExp.exec(/[A-Z]/, enumValue[i])) {\x0a                  // Replace myEnum-Foo with my_Enum-Foo:\x0a                  next = '_' + enumValue[i];\x0a                } else if ($RegExp.exec(/\W/, enumValue[i])) {\x0a                  // Replace my_Enum-Foo with my_Enum_Foo:\x0a                  next = '_';\x0a                } else {\x0a                  next = enumValue[i];\x0a                }\x0a                propertyName += next;\x0a              }\x0a              // Uppercase (replace my_Enum_Foo with MY_ENUM_FOO):\x0a              propertyName = $String.toUpperCase(propertyName);\x0a              mod[id][propertyName] = enumValue;\x0a            }\x0a          });\x0a        }\x0a      }, this);\x0a    }\x0a\x0a    // TODO(cduvall): Take out when all APIs have been converted to features.\x0a    // Returns whether access to the content of a schema should be denied,\x0a    // based on the presence of "unprivileged" and whether this is an\x0a    // extension process (versus e.g. a content script).\x0a    function isSchemaAccessAllowed(itemSchema) {\x0a      return (contextType == 'BLESSED_EXTENSION') ||\x0a             schema.unprivileged ||\x0a             itemSchema.unprivileged;\x0a    };\x0a\x0a    // Setup Functions.\x0a    if (schema.functions) {\x0a      $Array.forEach(schema.functions, function(functionDef) {\x0a        if (functionDef.name in mod) {\x0a          throw new Error('Function ' + functionDef.name +\x0a                          ' already defined in ' + schema.namespace);\x0a        }\x0a\x0a        if (!isSchemaNodeSupported(functionDef, platform, manifestVersion)) {\x0a          this.apiFunctions_.registerUnavailable(functionDef.name);\x0a          return;\x0a        }\x0a\x0a        var apiFunction = { __proto__: null };\x0a        apiFunction.definition = functionDef;\x0a        apiFunction.name = schema.namespace + '.' + functionDef.name;\x0a\x0a        if (!GetAvailability(apiFunction.name).is_available ||\x0a            (checkUnprivileged && !isSchemaAccessAllowed(functionDef))) {\x0a          this.apiFunctions_.registerUnavailable(functionDef.name);\x0a          return;\x0a        }\x0a\x0a        // TODO(aa): It would be best to run this in a unit test, but in order\x0a        // to do that we would need to better factor this code so that it\x0a        // doesn't depend on so much v8::Extension machinery.\x0a        if (logging.DCHECK_IS_ON() &&\x0a            schemaUtils.isFunctionSignatureAmbiguous(apiFunction.definition)) {\x0a          throw new Error(\x0a              apiFunction.name + ' has ambiguous optional arguments. ' +\x0a              'To implement custom disambiguation logic, add ' +\x0a              '"allowAmbiguousOptionalArguments" to the function\'s schema.');\x0a        }\x0a\x0a        this.apiFunctions_.register(functionDef.name, apiFunction);\x0a\x0a        mod[functionDef.name] = $Function.bind(function() {\x0a          var args = $Array.slice(arguments);\x0a          if (this.updateArgumentsPreValidate)\x0a            args = $Function.apply(this.updateArgumentsPreValidate, this, args);\x0a\x0a          args = schemaUtils.normalizeArgumentsAndValidate(args, this);\x0a          if (this.updateArgumentsPostValidate) {\x0a            args = $Function.apply(this.updateArgumentsPostValidate,\x0a                                   this,\x0a                                   args);\x0a          }\x0a\x0a          sendRequestHandler.clearCalledSendRequest();\x0a\x0a          var retval;\x0a          if (this.handleRequest) {\x0a            retval = $Function.apply(this.handleRequest, this, args);\x0a          } else {\x0a            var optArgs = {\x0a              __proto__: null,\x0a              customCallback: this.customCallback\x0a            };\x0a            retval = sendRequest(this.name, args,\x0a                                 this.definition.parameters,\x0a                                 optArgs);\x0a          }\x0a          sendRequestHandler.clearCalledSendRequest();\x0a\x0a          // Validate return value if in sanity check mode.\x0a          if (logging.DCHECK_IS_ON() && this.definition.returns)\x0a            schemaUtils.validate([retval], [this.definition.returns]);\x0a          return retval;\x0a        }, apiFunction);\x0a      }, this);\x0a    }\x0a\x0a    // Setup Events\x0a    if (schema.events) {\x0a      $Array.forEach(schema.events, function(eventDef) {\x0a        if (eventDef.name in mod) {\x0a          throw new Error('Event ' + eventDef.name +\x0a                          ' already defined in ' + schema.namespace);\x0a        }\x0a        if (!isSchemaNodeSupported(eventDef, platform, manifestVersion))\x0a          return;\x0a\x0a        var eventName = schema.namespace + "." + eventDef.name;\x0a        if (!GetAvailability(eventName).is_available ||\x0a            (checkUnprivileged && !isSchemaAccessAllowed(eventDef))) {\x0a          return;\x0a        }\x0a\x0a        var options = eventDef.options || {};\x0a        if (eventDef.filters && eventDef.filters.length > 0)\x0a          options.supportsFilters = true;\x0a\x0a        var parameters = eventDef.parameters;\x0a        if (this.customEvent_) {\x0a          mod[eventDef.name] = new this.customEvent_(\x0a              eventName, parameters, eventDef.extraParameters, options);\x0a        } else {\x0a          mod[eventDef.name] = new Event(eventName, parameters, options);\x0a        }\x0a      }, this);\x0a    }\x0a\x0a    function addProperties(m, parentDef) {\x0a      var properties = parentDef.properties;\x0a      if (!properties)\x0a        return;\x0a\x0a      forEach(properties, function(propertyName, propertyDef) {\x0a        if (propertyName in m)\x0a          return;  // TODO(kalman): be strict like functions/events somehow.\x0a        if (!isSchemaNodeSupported(propertyDef, platform, manifestVersion))\x0a          return;\x0a        if (!GetAvailability(schema.namespace + "." +\x0a              propertyName).is_available ||\x0a            (checkUnprivileged && !isSchemaAccessAllowed(propertyDef))) {\x0a          return;\x0a        }\x0a\x0a        // |value| is eventually added to |m|, the exposed API. Make copies\x0a        // of everything from the schema. (The schema is also frozen, so as long\x0a        // as we don't make any modifications, shallow copies are fine.)\x0a        var value;\x0a        if ($Array.isArray(propertyDef.value))\x0a          value = $Array.slice(propertyDef.value);\x0a        else if (typeof propertyDef.value === 'object')\x0a          value = $Object.assign({}, propertyDef.value);\x0a        else\x0a          value = p...

 query_str is:d543a38b0365f3544c9dd97d0009201a1e4d51e052274fbcdb0c7d0271896e71
Object::GetProperty Key is 0x2e74725a419 <String[1]: 1> Source code is:function generate() {\x0a    // NB: It's important to load the schema during generation rather than\x0a    // setting it beforehand so that we're more confident the schema we're\x0a    // loading is real, and not one that was injected by a page intercepting\x0a    // Binding.generate.\x0a    // Additionally, since the schema is an object returned from a native\x0a    // handler, its properties don't have the custom getters/setters that a page\x0a    // may have put on Object.prototype, and the object is frozen by v8.\x0a    var schema = schemaRegistry.GetSchema(this.apiName_);\x0a\x0a    function shouldCheckUnprivileged() {\x0a      var shouldCheck = 'unprivileged' in schema;\x0a      if (shouldCheck)\x0a        return shouldCheck;\x0a\x0a      $Array.forEach(['functions', 'events'], function(type) {\x0a        if ($Object.hasOwnProperty(schema, type)) {\x0a          $Array.forEach(schema[type], function(node) {\x0a            if ('unprivileged' in node)\x0a              shouldCheck = true;\x0a          });\x0a        }\x0a      });\x0a      if (shouldCheck)\x0a        return shouldCheck;\x0a\x0a      for (var property in schema.properties) {\x0a        if ($Object.hasOwnProperty(schema, property) &&\x0a            'unprivileged' in schema.properties[property]) {\x0a          shouldCheck = true;\x0a          break;\x0a        }\x0a      }\x0a      return shouldCheck;\x0a    }\x0a    var checkUnprivileged = shouldCheckUnprivileged();\x0a\x0a    // TODO(kalman/cduvall): Make GetAvailability handle this, then delete the\x0a    // supporting code.\x0a    if (!isSchemaNodeSupported(schema, platform, manifestVersion)) {\x0a      console.error('chrome.' + schema.namespace + ' is not supported on ' +\x0a                    'this platform or manifest version');\x0a      return undefined;\x0a    }\x0a\x0a    var mod = {};\x0a\x0a    var namespaces = $String.split(schema.namespace, '.');\x0a    for (var index = 0, name; name = namespaces[index]; index++) {\x0a      mod[name] = mod[name] || {};\x0a      mod = mod[name];\x0a    }\x0a\x0a    if (schema.types) {\x0a      $Array.forEach(schema.types, function(t) {\x0a        if (!isSchemaNodeSupported(t, platform, manifestVersion))\x0a          return;\x0a\x0a        // Add types to global schemaValidator; the types we depend on from\x0a        // other namespaces will be added as needed.\x0a        schemaUtils.schemaValidator.addTypes(t);\x0a\x0a        // Generate symbols for enums.\x0a        var enumValues = t['enum'];\x0a        if (enumValues) {\x0a          // Type IDs are qualified with the namespace during compilation,\x0a          // unfortunately, so remove it here.\x0a          logging.DCHECK($String.substr(t.id, 0, schema.namespace.length) ==\x0a                             schema.namespace);\x0a          // Note: + 1 because it ends in a '.', e.g., 'fooApi.Type'.\x0a          var id = $String.substr(t.id, schema.namespace.length + 1);\x0a          mod[id] = {};\x0a          $Array.forEach(enumValues, function(enumValue) {\x0a            // Note: enums can be declared either as a list of strings\x0a            // ['foo', 'bar'] or as a list of objects\x0a            // [{'name': 'foo'}, {'name': 'bar'}].\x0a            enumValue = $Object.hasOwnProperty(enumValue, 'name') ?\x0a                enumValue.name : enumValue;\x0a            if (enumValue) {  // Avoid setting any empty enums.\x0a              // Make all properties in ALL_CAPS_STYLE.\x0a              //\x0a              // The built-in versions of $String.replace call other built-ins,\x0a              // which may be clobbered. Instead, manually build the property\x0a              // name.\x0a              //\x0a              // If the first character is a digit (we know it must be one of\x0a              // a digit, a letter, or an underscore), precede it with an\x0a              // underscore.\x0a              var propertyName = ($RegExp.exec(/\d/, enumValue[0])) ? '_' : '';\x0a              for (var i = 0; i < enumValue.length; ++i) {\x0a                var next;\x0a                if (i > 0 && $RegExp.exec(/[a-z]/, enumValue[i-1]) &&\x0a                    $RegExp.exec(/[A-Z]/, enumValue[i])) {\x0a                  // Replace myEnum-Foo with my_Enum-Foo:\x0a                  next = '_' + enumValue[i];\x0a                } else if ($RegExp.exec(/\W/, enumValue[i])) {\x0a                  // Replace my_Enum-Foo with my_Enum_Foo:\x0a                  next = '_';\x0a                } else {\x0a                  next = enumValue[i];\x0a                }\x0a                propertyName += next;\x0a              }\x0a              // Uppercase (replace my_Enum_Foo with MY_ENUM_FOO):\x0a              propertyName = $String.toUpperCase(propertyName);\x0a              mod[id][propertyName] = enumValue;\x0a            }\x0a          });\x0a        }\x0a      }, this);\x0a    }\x0a\x0a    // TODO(cduvall): Take out when all APIs have been converted to features.\x0a    // Returns whether access to the content of a schema should be denied,\x0a    // based on the presence of "unprivileged" and whether this is an\x0a    // extension process (versus e.g. a content script).\x0a    function isSchemaAccessAllowed(itemSchema) {\x0a      return (contextType == 'BLESSED_EXTENSION') ||\x0a             schema.unprivileged ||\x0a             itemSchema.unprivileged;\x0a    };\x0a\x0a    // Setup Functions.\x0a    if (schema.functions) {\x0a      $Array.forEach(schema.functions, function(functionDef) {\x0a        if (functionDef.name in mod) {\x0a          throw new Error('Function ' + functionDef.name +\x0a                          ' already defined in ' + schema.namespace);\x0a        }\x0a\x0a        if (!isSchemaNodeSupported(functionDef, platform, manifestVersion)) {\x0a          this.apiFunctions_.registerUnavailable(functionDef.name);\x0a          return;\x0a        }\x0a\x0a        var apiFunction = { __proto__: null };\x0a        apiFunction.definition = functionDef;\x0a        apiFunction.name = schema.namespace + '.' + functionDef.name;\x0a\x0a        if (!GetAvailability(apiFunction.name).is_available ||\x0a            (checkUnprivileged && !isSchemaAccessAllowed(functionDef))) {\x0a          this.apiFunctions_.registerUnavailable(functionDef.name);\x0a          return;\x0a        }\x0a\x0a        // TODO(aa): It would be best to run this in a unit test, but in order\x0a        // to do that we would need to better factor this code so that it\x0a        // doesn't depend on so much v8::Extension machinery.\x0a        if (logging.DCHECK_IS_ON() &&\x0a            schemaUtils.isFunctionSignatureAmbiguous(apiFunction.definition)) {\x0a          throw new Error(\x0a              apiFunction.name + ' has ambiguous optional arguments. ' +\x0a              'To implement custom disambiguation logic, add ' +\x0a              '"allowAmbiguousOptionalArguments" to the function\'s schema.');\x0a        }\x0a\x0a        this.apiFunctions_.register(functionDef.name, apiFunction);\x0a\x0a        mod[functionDef.name] = $Function.bind(function() {\x0a          var args = $Array.slice(arguments);\x0a          if (this.updateArgumentsPreValidate)\x0a            args = $Function.apply(this.updateArgumentsPreValidate, this, args);\x0a\x0a          args = schemaUtils.normalizeArgumentsAndValidate(args, this);\x0a          if (this.updateArgumentsPostValidate) {\x0a            args = $Function.apply(this.updateArgumentsPostValidate,\x0a                                   this,\x0a                                   args);\x0a          }\x0a\x0a          sendRequestHandler.clearCalledSendRequest();\x0a\x0a          var retval;\x0a          if (this.handleRequest) {\x0a            retval = $Function.apply(this.handleRequest, this, args);\x0a          } else {\x0a            var optArgs = {\x0a              __proto__: null,\x0a              customCallback: this.customCallback\x0a            };\x0a            retval = sendRequest(this.name, args,\x0a                                 this.definition.parameters,\x0a                                 optArgs);\x0a          }\x0a          sendRequestHandler.clearCalledSendRequest();\x0a\x0a          // Validate return value if in sanity check mode.\x0a          if (logging.DCHECK_IS_ON() && this.definition.returns)\x0a            schemaUtils.validate([retval], [this.definition.returns]);\x0a          return retval;\x0a        }, apiFunction);\x0a      }, this);\x0a    }\x0a\x0a    // Setup Events\x0a    if (schema.events) {\x0a      $Array.forEach(schema.events, function(eventDef) {\x0a        if (eventDef.name in mod) {\x0a          throw new Error('Event ' + eventDef.name +\x0a                          ' already defined in ' + schema.namespace);\x0a        }\x0a        if (!isSchemaNodeSupported(eventDef, platform, manifestVersion))\x0a          return;\x0a\x0a        var eventName = schema.namespace + "." + eventDef.name;\x0a        if (!GetAvailability(eventName).is_available ||\x0a            (checkUnprivileged && !isSchemaAccessAllowed(eventDef))) {\x0a          return;\x0a        }\x0a\x0a        var options = eventDef.options || {};\x0a        if (eventDef.filters && eventDef.filters.length > 0)\x0a          options.supportsFilters = true;\x0a\x0a        var parameters = eventDef.parameters;\x0a        if (this.customEvent_) {\x0a          mod[eventDef.name] = new this.customEvent_(\x0a              eventName, parameters, eventDef.extraParameters, options);\x0a        } else {\x0a          mod[eventDef.name] = new Event(eventName, parameters, options);\x0a        }\x0a      }, this);\x0a    }\x0a\x0a    function addProperties(m, parentDef) {\x0a      var properties = parentDef.properties;\x0a      if (!properties)\x0a        return;\x0a\x0a      forEach(properties, function(propertyName, propertyDef) {\x0a        if (propertyName in m)\x0a          return;  // TODO(kalman): be strict like functions/events somehow.\x0a        if (!isSchemaNodeSupported(propertyDef, platform, manifestVersion))\x0a          return;\x0a        if (!GetAvailability(schema.namespace + "." +\x0a              propertyName).is_available ||\x0a            (checkUnprivileged && !isSchemaAccessAllowed(propertyDef))) {\x0a          return;\x0a        }\x0a\x0a        // |value| is eventually added to |m|, the exposed API. Make copies\x0a        // of everything from the schema. (The schema is also frozen, so as long\x0a        // as we don't make any modifications, shallow copies are fine.)\x0a        var value;\x0a        if ($Array.isArray(propertyDef.value))\x0a          value = $Array.slice(propertyDef.value);\x0a        else if (typeof propertyDef.value === 'object')\x0a          value = $Object.assign({}, propertyDef.value);\x0a        else\x0a          value = p...

 Line:256
 query_str is:d543a38b0365f3544c9dd97d0009201a1e4d51e052274fbcdb0c7d0271896e71
The query d543a38b0365f3544c9dd97d0009201a1e4d51e052274fbcdb0c7d0271896e71 is indeed a member!
Object::GetProperty Key is 0x2e74725a3f1 <String[1]: 2> Source code is:function generate() {\x0a    // NB: It's important to load the schema during generation rather than\x0a    // setting it beforehand so that we're more confident the schema we're\x0a    // loading is real, and not one that was injected by a page intercepting\x0a    // Binding.generate.\x0a    // Additionally, since the schema is an object returned from a native\x0a    // handler, its properties don't have the custom getters/setters that a page\x0a    // may have put on Object.prototype, and the object is frozen by v8.\x0a    var schema = schemaRegistry.GetSchema(this.apiName_);\x0a\x0a    function shouldCheckUnprivileged() {\x0a      var shouldCheck = 'unprivileged' in schema;\x0a      if (shouldCheck)\x0a        return shouldCheck;\x0a\x0a      $Array.forEach(['functions', 'events'], function(type) {\x0a        if ($Object.hasOwnProperty(schema, type)) {\x0a          $Array.forEach(schema[type], function(node) {\x0a            if ('unprivileged' in node)\x0a              shouldCheck = true;\x0a          });\x0a        }\x0a      });\x0a      if (shouldCheck)\x0a        return shouldCheck;\x0a\x0a      for (var property in schema.properties) {\x0a        if ($Object.hasOwnProperty(schema, property) &&\x0a            'unprivileged' in schema.properties[property]) {\x0a          shouldCheck = true;\x0a          break;\x0a        }\x0a      }\x0a      return shouldCheck;\x0a    }\x0a    var checkUnprivileged = shouldCheckUnprivileged();\x0a\x0a    // TODO(kalman/cduvall): Make GetAvailability handle this, then delete the\x0a    // supporting code.\x0a    if (!isSchemaNodeSupported(schema, platform, manifestVersion)) {\x0a      console.error('chrome.' + schema.namespace + ' is not supported on ' +\x0a                    'this platform or manifest version');\x0a      return undefined;\x0a    }\x0a\x0a    var mod = {};\x0a\x0a    var namespaces = $String.split(schema.namespace, '.');\x0a    for (var index = 0, name; name = namespaces[index]; index++) {\x0a      mod[name] = mod[name] || {};\x0a      mod = mod[name];\x0a    }\x0a\x0a    if (schema.types) {\x0a      $Array.forEach(schema.types, function(t) {\x0a        if (!isSchemaNodeSupported(t, platform, manifestVersion))\x0a          return;\x0a\x0a        // Add types to global schemaValidator; the types we depend on from\x0a        // other namespaces will be added as needed.\x0a        schemaUtils.schemaValidator.addTypes(t);\x0a\x0a        // Generate symbols for enums.\x0a        var enumValues = t['enum'];\x0a        if (enumValues) {\x0a          // Type IDs are qualified with the namespace during compilation,\x0a          // unfortunately, so remove it here.\x0a          logging.DCHECK($String.substr(t.id, 0, schema.namespace.length) ==\x0a                             schema.namespace);\x0a          // Note: + 1 because it ends in a '.', e.g., 'fooApi.Type'.\x0a          var id = $String.substr(t.id, schema.namespace.length + 1);\x0a          mod[id] = {};\x0a          $Array.forEach(enumValues, function(enumValue) {\x0a            // Note: enums can be declared either as a list of strings\x0a            // ['foo', 'bar'] or as a list of objects\x0a            // [{'name': 'foo'}, {'name': 'bar'}].\x0a            enumValue = $Object.hasOwnProperty(enumValue, 'name') ?\x0a                enumValue.name : enumValue;\x0a            if (enumValue) {  // Avoid setting any empty enums.\x0a              // Make all properties in ALL_CAPS_STYLE.\x0a              //\x0a              // The built-in versions of $String.replace call other built-ins,\x0a              // which may be clobbered. Instead, manually build the property\x0a              // name.\x0a              //\x0a              // If the first character is a digit (we know it must be one of\x0a              // a digit, a letter, or an underscore), precede it with an\x0a              // underscore.\x0a              var propertyName = ($RegExp.exec(/\d/, enumValue[0])) ? '_' : '';\x0a              for (var i = 0; i < enumValue.length; ++i) {\x0a                var next;\x0a                if (i > 0 && $RegExp.exec(/[a-z]/, enumValue[i-1]) &&\x0a                    $RegExp.exec(/[A-Z]/, enumValue[i])) {\x0a                  // Replace myEnum-Foo with my_Enum-Foo:\x0a                  next = '_' + enumValue[i];\x0a                } else if ($RegExp.exec(/\W/, enumValue[i])) {\x0a                  // Replace my_Enum-Foo with my_Enum_Foo:\x0a                  next = '_';\x0a                } else {\x0a                  next = enumValue[i];\x0a                }\x0a                propertyName += next;\x0a              }\x0a              // Uppercase (replace my_Enum_Foo with MY_ENUM_FOO):\x0a              propertyName = $String.toUpperCase(propertyName);\x0a              mod[id][propertyName] = enumValue;\x0a            }\x0a          });\x0a        }\x0a      }, this);\x0a    }\x0a\x0a    // TODO(cduvall): Take out when all APIs have been converted to features.\x0a    // Returns whether access to the content of a schema should be denied,\x0a    // based on the presence of "unprivileged" and whether this is an\x0a    // extension process (versus e.g. a content script).\x0a    function isSchemaAccessAllowed(itemSchema) {\x0a      return (contextType == 'BLESSED_EXTENSION') ||\x0a             schema.unprivileged ||\x0a             itemSchema.unprivileged;\x0a    };\x0a\x0a    // Setup Functions.\x0a    if (schema.functions) {\x0a      $Array.forEach(schema.functions, function(functionDef) {\x0a        if (functionDef.name in mod) {\x0a          throw new Error('Function ' + functionDef.name +\x0a                          ' already defined in ' + schema.namespace);\x0a        }\x0a\x0a        if (!isSchemaNodeSupported(functionDef, platform, manifestVersion)) {\x0a          this.apiFunctions_.registerUnavailable(functionDef.name);\x0a          return;\x0a        }\x0a\x0a        var apiFunction = { __proto__: null };\x0a        apiFunction.definition = functionDef;\x0a        apiFunction.name = schema.namespace + '.' + functionDef.name;\x0a\x0a        if (!GetAvailability(apiFunction.name).is_available ||\x0a            (checkUnprivileged && !isSchemaAccessAllowed(functionDef))) {\x0a          this.apiFunctions_.registerUnavailable(functionDef.name);\x0a          return;\x0a        }\x0a\x0a        // TODO(aa): It would be best to run this in a unit test, but in order\x0a        // to do that we would need to better factor this code so that it\x0a        // doesn't depend on so much v8::Extension machinery.\x0a        if (logging.DCHECK_IS_ON() &&\x0a            schemaUtils.isFunctionSignatureAmbiguous(apiFunction.definition)) {\x0a          throw new Error(\x0a              apiFunction.name + ' has ambiguous optional arguments. ' +\x0a              'To implement custom disambiguation logic, add ' +\x0a              '"allowAmbiguousOptionalArguments" to the function\'s schema.');\x0a        }\x0a\x0a        this.apiFunctions_.register(functionDef.name, apiFunction);\x0a\x0a        mod[functionDef.name] = $Function.bind(function() {\x0a          var args = $Array.slice(arguments);\x0a          if (this.updateArgumentsPreValidate)\x0a            args = $Function.apply(this.updateArgumentsPreValidate, this, args);\x0a\x0a          args = schemaUtils.normalizeArgumentsAndValidate(args, this);\x0a          if (this.updateArgumentsPostValidate) {\x0a            args = $Function.apply(this.updateArgumentsPostValidate,\x0a                                   this,\x0a                                   args);\x0a          }\x0a\x0a          sendRequestHandler.clearCalledSendRequest();\x0a\x0a          var retval;\x0a          if (this.handleRequest) {\x0a            retval = $Function.apply(this.handleRequest, this, args);\x0a          } else {\x0a            var optArgs = {\x0a              __proto__: null,\x0a              customCallback: this.customCallback\x0a            };\x0a            retval = sendRequest(this.name, args,\x0a                                 this.definition.parameters,\x0a                                 optArgs);\x0a          }\x0a          sendRequestHandler.clearCalledSendRequest();\x0a\x0a          // Validate return value if in sanity check mode.\x0a          if (logging.DCHECK_IS_ON() && this.definition.returns)\x0a            schemaUtils.validate([retval], [this.definition.returns]);\x0a          return retval;\x0a        }, apiFunction);\x0a      }, this);\x0a    }\x0a\x0a    // Setup Events\x0a    if (schema.events) {\x0a      $Array.forEach(schema.events, function(eventDef) {\x0a        if (eventDef.name in mod) {\x0a          throw new Error('Event ' + eventDef.name +\x0a                          ' already defined in ' + schema.namespace);\x0a        }\x0a        if (!isSchemaNodeSupported(eventDef, platform, manifestVersion))\x0a          return;\x0a\x0a        var eventName = schema.namespace + "." + eventDef.name;\x0a        if (!GetAvailability(eventName).is_available ||\x0a            (checkUnprivileged && !isSchemaAccessAllowed(eventDef))) {\x0a          return;\x0a        }\x0a\x0a        var options = eventDef.options || {};\x0a        if (eventDef.filters && eventDef.filters.length > 0)\x0a          options.supportsFilters = true;\x0a\x0a        var parameters = eventDef.parameters;\x0a        if (this.customEvent_) {\x0a          mod[eventDef.name] = new this.customEvent_(\x0a              eventName, parameters, eventDef.extraParameters, options);\x0a        } else {\x0a          mod[eventDef.name] = new Event(eventName, parameters, options);\x0a        }\x0a      }, this);\x0a    }\x0a\x0a    function addProperties(m, parentDef) {\x0a      var properties = parentDef.properties;\x0a      if (!properties)\x0a        return;\x0a\x0a      forEach(properties, function(propertyName, propertyDef) {\x0a        if (propertyName in m)\x0a          return;  // TODO(kalman): be strict like functions/events somehow.\x0a        if (!isSchemaNodeSupported(propertyDef, platform, manifestVersion))\x0a          return;\x0a        if (!GetAvailability(schema.namespace + "." +\x0a              propertyName).is_available ||\x0a            (checkUnprivileged && !isSchemaAccessAllowed(propertyDef))) {\x0a          return;\x0a        }\x0a\x0a        // |value| is eventually added to |m|, the exposed API. Make copies\x0a        // of everything from the schema. (The schema is also frozen, so as long\x0a        // as we don't make any modifications, shallow copies are fine.)\x0a        var value;\x0a        if ($Array.isArray(propertyDef.value))\x0a          value = $Array.slice(propertyDef.value);\x0a        else if (typeof propertyDef.value === 'object')\x0a          value = $Object.assign({}, propertyDef.value);\x0a        else\x0a          value = p...

 Line:256
 query_str is:d543a38b0365f3544c9dd97d0009201a1e4d51e052274fbcdb0c7d0271896e71
The query d543a38b0365f3544c9dd97d0009201a1e4d51e052274fbcdb0c7d0271896e71 is indeed a member!
Object::GetProperty Key is 0x2e7472eab19 <String[1]: 0> Source code is:function generate() {\x0a    // NB: It's important to load the schema during generation rather than\x0a    // setting it beforehand so that we're more confident the schema we're\x0a    // loading is real, and not one that was injected by a page intercepting\x0a    // Binding.generate.\x0a    // Additionally, since the schema is an object returned from a native\x0a    // handler, its properties don't have the custom getters/setters that a page\x0a    // may have put on Object.prototype, and the object is frozen by v8.\x0a    var schema = schemaRegistry.GetSchema(this.apiName_);\x0a\x0a    function shouldCheckUnprivileged() {\x0a      var shouldCheck = 'unprivileged' in schema;\x0a      if (shouldCheck)\x0a        return shouldCheck;\x0a\x0a      $Array.forEach(['functions', 'events'], function(type) {\x0a        if ($Object.hasOwnProperty(schema, type)) {\x0a          $Array.forEach(schema[type], function(node) {\x0a            if ('unprivileged' in node)\x0a              shouldCheck = true;\x0a          });\x0a        }\x0a      });\x0a      if (shouldCheck)\x0a        return shouldCheck;\x0a\x0a      for (var property in schema.properties) {\x0a        if ($Object.hasOwnProperty(schema, property) &&\x0a            'unprivileged' in schema.properties[property]) {\x0a          shouldCheck = true;\x0a          break;\x0a        }\x0a      }\x0a      return shouldCheck;\x0a    }\x0a    var checkUnprivileged = shouldCheckUnprivileged();\x0a\x0a    // TODO(kalman/cduvall): Make GetAvailability handle this, then delete the\x0a    // supporting code.\x0a    if (!isSchemaNodeSupported(schema, platform, manifestVersion)) {\x0a      console.error('chrome.' + schema.namespace + ' is not supported on ' +\x0a                    'this platform or manifest version');\x0a      return undefined;\x0a    }\x0a\x0a    var mod = {};\x0a\x0a    var namespaces = $String.split(schema.namespace, '.');\x0a    for (var index = 0, name; name = namespaces[index]; index++) {\x0a      mod[name] = mod[name] || {};\x0a      mod = mod[name];\x0a    }\x0a\x0a    if (schema.types) {\x0a      $Array.forEach(schema.types, function(t) {\x0a        if (!isSchemaNodeSupported(t, platform, manifestVersion))\x0a          return;\x0a\x0a        // Add types to global schemaValidator; the types we depend on from\x0a        // other namespaces will be added as needed.\x0a        schemaUtils.schemaValidator.addTypes(t);\x0a\x0a        // Generate symbols for enums.\x0a        var enumValues = t['enum'];\x0a        if (enumValues) {\x0a          // Type IDs are qualified with the namespace during compilation,\x0a          // unfortunately, so remove it here.\x0a          logging.DCHECK($String.substr(t.id, 0, schema.namespace.length) ==\x0a                             schema.namespace);\x0a          // Note: + 1 because it ends in a '.', e.g., 'fooApi.Type'.\x0a          var id = $String.substr(t.id, schema.namespace.length + 1);\x0a          mod[id] = {};\x0a          $Array.forEach(enumValues, function(enumValue) {\x0a            // Note: enums can be declared either as a list of strings\x0a            // ['foo', 'bar'] or as a list of objects\x0a            // [{'name': 'foo'}, {'name': 'bar'}].\x0a            enumValue = $Object.hasOwnProperty(enumValue, 'name') ?\x0a                enumValue.name : enumValue;\x0a            if (enumValue) {  // Avoid setting any empty enums.\x0a              // Make all properties in ALL_CAPS_STYLE.\x0a              //\x0a              // The built-in versions of $String.replace call other built-ins,\x0a              // which may be clobbered. Instead, manually build the property\x0a              // name.\x0a              //\x0a              // If the first character is a digit (we know it must be one of\x0a              // a digit, a letter, or an underscore), precede it with an\x0a              // underscore.\x0a              var propertyName = ($RegExp.exec(/\d/, enumValue[0])) ? '_' : '';\x0a              for (var i = 0; i < enumValue.length; ++i) {\x0a                var next;\x0a                if (i > 0 && $RegExp.exec(/[a-z]/, enumValue[i-1]) &&\x0a                    $RegExp.exec(/[A-Z]/, enumValue[i])) {\x0a                  // Replace myEnum-Foo with my_Enum-Foo:\x0a                  next = '_' + enumValue[i];\x0a                } else if ($RegExp.exec(/\W/, enumValue[i])) {\x0a                  // Replace my_Enum-Foo with my_Enum_Foo:\x0a                  next = '_';\x0a                } else {\x0a                  next = enumValue[i];\x0a                }\x0a                propertyName += next;\x0a              }\x0a              // Uppercase (replace my_Enum_Foo with MY_ENUM_FOO):\x0a              propertyName = $String.toUpperCase(propertyName);\x0a              mod[id][propertyName] = enumValue;\x0a            }\x0a          });\x0a        }\x0a      }, this);\x0a    }\x0a\x0a    // TODO(cduvall): Take out when all APIs have been converted to features.\x0a    // Returns whether access to the content of a schema should be denied,\x0a    // based on the presence of "unprivileged" and whether this is an\x0a    // extension process (versus e.g. a content script).\x0a    function isSchemaAccessAllowed(itemSchema) {\x0a      return (contextType == 'BLESSED_EXTENSION') ||\x0a             schema.unprivileged ||\x0a             itemSchema.unprivileged;\x0a    };\x0a\x0a    // Setup Functions.\x0a    if (schema.functions) {\x0a      $Array.forEach(schema.functions, function(functionDef) {\x0a        if (functionDef.name in mod) {\x0a          throw new Error('Function ' + functionDef.name +\x0a                          ' already defined in ' + schema.namespace);\x0a        }\x0a\x0a        if (!isSchemaNodeSupported(functionDef, platform, manifestVersion)) {\x0a          this.apiFunctions_.registerUnavailable(functionDef.name);\x0a          return;\x0a        }\x0a\x0a        var apiFunction = { __proto__: null };\x0a        apiFunction.definition = functionDef;\x0a        apiFunction.name = schema.namespace + '.' + functionDef.name;\x0a\x0a        if (!GetAvailability(apiFunction.name).is_available ||\x0a            (checkUnprivileged && !isSchemaAccessAllowed(functionDef))) {\x0a          this.apiFunctions_.registerUnavailable(functionDef.name);\x0a          return;\x0a        }\x0a\x0a        // TODO(aa): It would be best to run this in a unit test, but in order\x0a        // to do that we would need to better factor this code so that it\x0a        // doesn't depend on so much v8::Extension machinery.\x0a        if (logging.DCHECK_IS_ON() &&\x0a            schemaUtils.isFunctionSignatureAmbiguous(apiFunction.definition)) {\x0a          throw new Error(\x0a              apiFunction.name + ' has ambiguous optional arguments. ' +\x0a              'To implement custom disambiguation logic, add ' +\x0a              '"allowAmbiguousOptionalArguments" to the function\'s schema.');\x0a        }\x0a\x0a        this.apiFunctions_.register(functionDef.name, apiFunction);\x0a\x0a        mod[functionDef.name] = $Function.bind(function() {\x0a          var args = $Array.slice(arguments);\x0a          if (this.updateArgumentsPreValidate)\x0a            args = $Function.apply(this.updateArgumentsPreValidate, this, args);\x0a\x0a          args = schemaUtils.normalizeArgumentsAndValidate(args, this);\x0a          if (this.updateArgumentsPostValidate) {\x0a            args = $Function.apply(this.updateArgumentsPostValidate,\x0a                                   this,\x0a                                   args);\x0a          }\x0a\x0a          sendRequestHandler.clearCalledSendRequest();\x0a\x0a          var retval;\x0a          if (this.handleRequest) {\x0a            retval = $Function.apply(this.handleRequest, this, args);\x0a          } else {\x0a            var optArgs = {\x0a              __proto__: null,\x0a              customCallback: this.customCallback\x0a            };\x0a            retval = sendRequest(this.name, args,\x0a                                 this.definition.parameters,\x0a                                 optArgs);\x0a          }\x0a          sendRequestHandler.clearCalledSendRequest();\x0a\x0a          // Validate return value if in sanity check mode.\x0a          if (logging.DCHECK_IS_ON() && this.definition.returns)\x0a            schemaUtils.validate([retval], [this.definition.returns]);\x0a          return retval;\x0a        }, apiFunction);\x0a      }, this);\x0a    }\x0a\x0a    // Setup Events\x0a    if (schema.events) {\x0a      $Array.forEach(schema.events, function(eventDef) {\x0a        if (eventDef.name in mod) {\x0a          throw new Error('Event ' + eventDef.name +\x0a                          ' already defined in ' + schema.namespace);\x0a        }\x0a        if (!isSchemaNodeSupported(eventDef, platform, manifestVersion))\x0a          return;\x0a\x0a        var eventName = schema.namespace + "." + eventDef.name;\x0a        if (!GetAvailability(eventName).is_available ||\x0a            (checkUnprivileged && !isSchemaAccessAllowed(eventDef))) {\x0a          return;\x0a        }\x0a\x0a        var options = eventDef.options || {};\x0a        if (eventDef.filters && eventDef.filters.length > 0)\x0a          options.supportsFilters = true;\x0a\x0a        var parameters = eventDef.parameters;\x0a        if (this.customEvent_) {\x0a          mod[eventDef.name] = new this.customEvent_(\x0a              eventName, parameters, eventDef.extraParameters, options);\x0a        } else {\x0a          mod[eventDef.name] = new Event(eventName, parameters, options);\x0a        }\x0a      }, this);\x0a    }\x0a\x0a    function addProperties(m, parentDef) {\x0a      var properties = parentDef.properties;\x0a      if (!properties)\x0a        return;\x0a\x0a      forEach(properties, function(propertyName, propertyDef) {\x0a        if (propertyName in m)\x0a          return;  // TODO(kalman): be strict like functions/events somehow.\x0a        if (!isSchemaNodeSupported(propertyDef, platform, manifestVersion))\x0a          return;\x0a        if (!GetAvailability(schema.namespace + "." +\x0a              propertyName).is_available ||\x0a            (checkUnprivileged && !isSchemaAccessAllowed(propertyDef))) {\x0a          return;\x0a        }\x0a\x0a        // |value| is eventually added to |m|, the exposed API. Make copies\x0a        // of everything from the schema. (The schema is also frozen, so as long\x0a        // as we don't make any modifications, shallow copies are fine.)\x0a        var value;\x0a        if ($Array.isArray(propertyDef.value))\x0a          value = $Array.slice(propertyDef.value);\x0a        else if (typeof propertyDef.value === 'object')\x0a          value = $Object.assign({}, propertyDef.value);\x0a        else\x0a          value = p...

 Line:256
 query_str is:d543a38b0365f3544c9dd97d0009201a1e4d51e052274fbcdb0c7d0271896e71
The query d543a38b0365f3544c9dd97d0009201a1e4d51e052274fbcdb0c7d0271896e71 is indeed a member!
Object::GetProperty Key is 0x2e7472eab19 <String[1]: 0> Source code is:function generate() {\x0a    // NB: It's important to load the schema during generation rather than\x0a    // setting it beforehand so that we're more confident the schema we're\x0a    // loading is real, and not one that was injected by a page intercepting\x0a    // Binding.generate.\x0a    // Additionally, since the schema is an object returned from a native\x0a    // handler, its properties don't have the custom getters/setters that a page\x0a    // may have put on Object.prototype, and the object is frozen by v8.\x0a    var schema = schemaRegistry.GetSchema(this.apiName_);\x0a\x0a    function shouldCheckUnprivileged() {\x0a      var shouldCheck = 'unprivileged' in schema;\x0a      if (shouldCheck)\x0a        return shouldCheck;\x0a\x0a      $Array.forEach(['functions', 'events'], function(type) {\x0a        if ($Object.hasOwnProperty(schema, type)) {\x0a          $Array.forEach(schema[type], function(node) {\x0a            if ('unprivileged' in node)\x0a              shouldCheck = true;\x0a          });\x0a        }\x0a      });\x0a      if (shouldCheck)\x0a        return shouldCheck;\x0a\x0a      for (var property in schema.properties) {\x0a        if ($Object.hasOwnProperty(schema, property) &&\x0a            'unprivileged' in schema.properties[property]) {\x0a          shouldCheck = true;\x0a          break;\x0a        }\x0a      }\x0a      return shouldCheck;\x0a    }\x0a    var checkUnprivileged = shouldCheckUnprivileged();\x0a\x0a    // TODO(kalman/cduvall): Make GetAvailability handle this, then delete the\x0a    // supporting code.\x0a    if (!isSchemaNodeSupported(schema, platform, manifestVersion)) {\x0a      console.error('chrome.' + schema.namespace + ' is not supported on ' +\x0a                    'this platform or manifest version');\x0a      return undefined;\x0a    }\x0a\x0a    var mod = {};\x0a\x0a    var namespaces = $String.split(schema.namespace, '.');\x0a    for (var index = 0, name; name = namespaces[index]; index++) {\x0a      mod[name] = mod[name] || {};\x0a      mod = mod[name];\x0a    }\x0a\x0a    if (schema.types) {\x0a      $Array.forEach(schema.types, function(t) {\x0a        if (!isSchemaNodeSupported(t, platform, manifestVersion))\x0a          return;\x0a\x0a        // Add types to global schemaValidator; the types we depend on from\x0a        // other namespaces will be added as needed.\x0a        schemaUtils.schemaValidator.addTypes(t);\x0a\x0a        // Generate symbols for enums.\x0a        var enumValues = t['enum'];\x0a        if (enumValues) {\x0a          // Type IDs are qualified with the namespace during compilation,\x0a          // unfortunately, so remove it here.\x0a          logging.DCHECK($String.substr(t.id, 0, schema.namespace.length) ==\x0a                             schema.namespace);\x0a          // Note: + 1 because it ends in a '.', e.g., 'fooApi.Type'.\x0a          var id = $String.substr(t.id, schema.namespace.length + 1);\x0a          mod[id] = {};\x0a          $Array.forEach(enumValues, function(enumValue) {\x0a            // Note: enums can be declared either as a list of strings\x0a            // ['foo', 'bar'] or as a list of objects\x0a            // [{'name': 'foo'}, {'name': 'bar'}].\x0a            enumValue = $Object.hasOwnProperty(enumValue, 'name') ?\x0a                enumValue.name : enumValue;\x0a            if (enumValue) {  // Avoid setting any empty enums.\x0a              // Make all properties in ALL_CAPS_STYLE.\x0a              //\x0a              // The built-in versions of $String.replace call other built-ins,\x0a              // which may be clobbered. Instead, manually build the property\x0a              // name.\x0a              //\x0a              // If the first character is a digit (we know it must be one of\x0a              // a digit, a letter, or an underscore), precede it with an\x0a              // underscore.\x0a              var propertyName = ($RegExp.exec(/\d/, enumValue[0])) ? '_' : '';\x0a              for (var i = 0; i < enumValue.length; ++i) {\x0a                var next;\x0a                if (i > 0 && $RegExp.exec(/[a-z]/, enumValue[i-1]) &&\x0a                    $RegExp.exec(/[A-Z]/, enumValue[i])) {\x0a                  // Replace myEnum-Foo with my_Enum-Foo:\x0a                  next = '_' + enumValue[i];\x0a                } else if ($RegExp.exec(/\W/, enumValue[i])) {\x0a                  // Replace my_Enum-Foo with my_Enum_Foo:\x0a                  next = '_';\x0a                } else {\x0a                  next = enumValue[i];\x0a                }\x0a                propertyName += next;\x0a              }\x0a              // Uppercase (replace my_Enum_Foo with MY_ENUM_FOO):\x0a              propertyName = $String.toUpperCase(propertyName);\x0a              mod[id][propertyName] = enumValue;\x0a            }\x0a          });\x0a        }\x0a      }, this);\x0a    }\x0a\x0a    // TODO(cduvall): Take out when all APIs have been converted to features.\x0a    // Returns whether access to the content of a schema should be denied,\x0a    // based on the presence of "unprivileged" and whether this is an\x0a    // extension process (versus e.g. a content script).\x0a    function isSchemaAccessAllowed(itemSchema) {\x0a      return (contextType == 'BLESSED_EXTENSION') ||\x0a             schema.unprivileged ||\x0a             itemSchema.unprivileged;\x0a    };\x0a\x0a    // Setup Functions.\x0a    if (schema.functions) {\x0a      $Array.forEach(schema.functions, function(functionDef) {\x0a        if (functionDef.name in mod) {\x0a          throw new Error('Function ' + functionDef.name +\x0a                          ' already defined in ' + schema.namespace);\x0a        }\x0a\x0a        if (!isSchemaNodeSupported(functionDef, platform, manifestVersion)) {\x0a          this.apiFunctions_.registerUnavailable(functionDef.name);\x0a          return;\x0a        }\x0a\x0a        var apiFunction = { __proto__: null };\x0a        apiFunction.definition = functionDef;\x0a        apiFunction.name = schema.namespace + '.' + functionDef.name;\x0a\x0a        if (!GetAvailability(apiFunction.name).is_available ||\x0a            (checkUnprivileged && !isSchemaAccessAllowed(functionDef))) {\x0a          this.apiFunctions_.registerUnavailable(functionDef.name);\x0a          return;\x0a        }\x0a\x0a        // TODO(aa): It would be best to run this in a unit test, but in order\x0a        // to do that we would need to better factor this code so that it\x0a        // doesn't depend on so much v8::Extension machinery.\x0a        if (logging.DCHECK_IS_ON() &&\x0a            schemaUtils.isFunctionSignatureAmbiguous(apiFunction.definition)) {\x0a          throw new Error(\x0a              apiFunction.name + ' has ambiguous optional arguments. ' +\x0a              'To implement custom disambiguation logic, add ' +\x0a              '"allowAmbiguousOptionalArguments" to the function\'s schema.');\x0a        }\x0a\x0a        this.apiFunctions_.register(functionDef.name, apiFunction);\x0a\x0a        mod[functionDef.name] = $Function.bind(function() {\x0a          var args = $Array.slice(arguments);\x0a          if (this.updateArgumentsPreValidate)\x0a            args = $Function.apply(this.updateArgumentsPreValidate, this, args);\x0a\x0a          args = schemaUtils.normalizeArgumentsAndValidate(args, this);\x0a          if (this.updateArgumentsPostValidate) {\x0a            args = $Function.apply(this.updateArgumentsPostValidate,\x0a                                   this,\x0a                                   args);\x0a          }\x0a\x0a          sendRequestHandler.clearCalledSendRequest();\x0a\x0a          var retval;\x0a          if (this.handleRequest) {\x0a            retval = $Function.apply(this.handleRequest, this, args);\x0a          } else {\x0a            var optArgs = {\x0a              __proto__: null,\x0a              customCallback: this.customCallback\x0a            };\x0a            retval = sendRequest(this.name, args,\x0a                                 this.definition.parameters,\x0a                                 optArgs);\x0a          }\x0a          sendRequestHandler.clearCalledSendRequest();\x0a\x0a          // Validate return value if in sanity check mode.\x0a          if (logging.DCHECK_IS_ON() && this.definition.returns)\x0a            schemaUtils.validate([retval], [this.definition.returns]);\x0a          return retval;\x0a        }, apiFunction);\x0a      }, this);\x0a    }\x0a\x0a    // Setup Events\x0a    if (schema.events) {\x0a      $Array.forEach(schema.events, function(eventDef) {\x0a        if (eventDef.name in mod) {\x0a          throw new Error('Event ' + eventDef.name +\x0a                          ' already defined in ' + schema.namespace);\x0a        }\x0a        if (!isSchemaNodeSupported(eventDef, platform, manifestVersion))\x0a          return;\x0a\x0a        var eventName = schema.namespace + "." + eventDef.name;\x0a        if (!GetAvailability(eventName).is_available ||\x0a            (checkUnprivileged && !isSchemaAccessAllowed(eventDef))) {\x0a          return;\x0a        }\x0a\x0a        var options = eventDef.options || {};\x0a        if (eventDef.filters && eventDef.filters.length > 0)\x0a          options.supportsFilters = true;\x0a\x0a        var parameters = eventDef.parameters;\x0a        if (this.customEvent_) {\x0a          mod[eventDef.name] = new this.customEvent_(\x0a              eventName, parameters, eventDef.extraParameters, options);\x0a        } else {\x0a          mod[eventDef.name] = new Event(eventName, parameters, options);\x0a        }\x0a      }, this);\x0a    }\x0a\x0a    function addProperties(m, parentDef) {\x0a      var properties = parentDef.properties;\x0a      if (!properties)\x0a        return;\x0a\x0a      forEach(properties, function(propertyName, propertyDef) {\x0a        if (propertyName in m)\x0a          return;  // TODO(kalman): be strict like functions/events somehow.\x0a        if (!isSchemaNodeSupported(propertyDef, platform, manifestVersion))\x0a          return;\x0a        if (!GetAvailability(schema.namespace + "." +\x0a              propertyName).is_available ||\x0a            (checkUnprivileged && !isSchemaAccessAllowed(propertyDef))) {\x0a          return;\x0a        }\x0a\x0a        // |value| is eventually added to |m|, the exposed API. Make copies\x0a        // of everything from the schema. (The schema is also frozen, so as long\x0a        // as we don't make any modifications, shallow copies are fine.)\x0a        var value;\x0a        if ($Array.isArray(propertyDef.value))\x0a          value = $Array.slice(propertyDef.value);\x0a        else if (typeof propertyDef.value === 'object')\x0a          value = $Object.assign({}, propertyDef.value);\x0a        else\x0a          value = p...

 Line:256
 query_str is:d543a38b0365f3544c9dd97d0009201a1e4d51e052274fbcdb0c7d0271896e71
The query d543a38b0365f3544c9dd97d0009201a1e4d51e052274fbcdb0c7d0271896e71 is indeed a member!
Object::GetProperty Key is 0x2e74725a419 <String[1]: 1> Source code is:function generate() {\x0a    // NB: It's important to load the schema during generation rather than\x0a    // setting it beforehand so that we're more confident the schema we're\x0a    // loading is real, and not one that was injected by a page intercepting\x0a    // Binding.generate.\x0a    // Additionally, since the schema is an object returned from a native\x0a    // handler, its properties don't have the custom getters/setters that a page\x0a    // may have put on Object.prototype, and the object is frozen by v8.\x0a    var schema = schemaRegistry.GetSchema(this.apiName_);\x0a\x0a    function shouldCheckUnprivileged() {\x0a      var shouldCheck = 'unprivileged' in schema;\x0a      if (shouldCheck)\x0a        return shouldCheck;\x0a\late void CopyIn<SeqString>(SeqString*, const TaintData*, int, int);

template void CopyOut<SeqString>(SeqString*, TaintData*, int, int);
template void CopyOut<SeqOneByteString>(
    SeqOneByteString*, TaintData*, int, int);
template void CopyOut<SeqTwoByteString>(
    SeqTwoByteString*, TaintData*, int, int);

template void OnNewReplaceRegexpWithString<SeqOneByteString>(
    String* subject, SeqOneByteString* result, JSRegExp* pattern,
    String* replacement);
template void OnNewReplaceRegexpWithString<SeqTwoByteString>(
    String* subject, SeqTwoByteString* result, JSRegExp* pattern,
    String* replacement);

template void OnJoinManyStrings<SeqOneByteString, JSArray>(
    SeqOneByteString*, JSArray*);
template void OnJoinManyStrings<SeqTwoByteString, JSArray>(
    SeqTwoByteString*, JSArray*);
template void OnJoinManyStrings<SeqOneByteString, FixedArray>(
    SeqOneByteString*, FixedArray*);
template void OnJoinManyStrings<SeqTwoByteString, FixedArray>(
    SeqTwoByteString*, FixedArray*);

template void FlattenTaint<SeqOneByteString, String>(
    String*, SeqOneByteString*, int, int);
template void FlattenTaint<SeqTwoByteString, String>(
    String*, SeqTwoByteString*, int, int);



template <size_t N>
void LogSymbolic(String* first,
                 const std::array<String*, N>& refs,
                 std::string extra,
                 SymbolicType type) {
  DCHECK(FLAG_taint_tracking_enable_symbolic);
  DCHECK_NOT_NULL(first);

  Isolate* isolate = first->GetIsolate();
  MessageHolder message;
  auto log_message = message.InitRoot();
  auto symbolic_log = log_message.getMessage().initSymbolicLog();
  symbolic_log.setTargetId(first->taint_info());
  auto arg_list = symbolic_log.initArgRefs(refs.size());
  for (int i = 0; i < refs.size(); i++) {
    arg_list.set(i, refs[i]->taint_info());
  }
  message.CopyJsStringSlow(symbolic_log.initTargetValue(), first);
  IsTaintedVisitor visitor;
  visitor.run(first, 0, first->length());
  auto info_ranges = visitor.GetRanges();
  auto value = symbolic_log.initTaintValue();
  InitTaintInfo(info_ranges, &value);
  symbolic_log.setSymbolicOperation(SymbolicTypeToEnum(type));

  TaintTracker::Impl::LogToFile(isolate, message);
}


template <class T> void OnNewStringLiteral(T* source) {
  if (FLAG_taint_tracking_enable_symbolic) {
    LogSymbolic<0>(source, {{}}, "", LITERAL);
  }
}
template void OnNewStringLiteral(String* source);
template void OnNewStringLiteral(SeqOneByteString* source);
template void OnNewStringLiteral(SeqTwoByteString* source);

// template <> void Print_String_Helper<SeqOneByteString>(SeqOneByteString* source, int depth) {
//     StringShape shape(source);
//     DCHECK(!shape.IsCons());
//     Vector<const uint8_t> str = source->GetCharVector<uint8_t>();
//       for (auto i = str.begin(); i != str.end(); ++i){
//         std::cout << *i;
//           }
//   // }
// }

// template <> void Print_String_Helper<SeqTwoByteString>(SeqTwoByteString* source, int depth) {
//     StringShape shape(source);
//     DCHECK(!shape.IsCons());
//     Vector<const uc16> str = source->GetCharVector<uc16>();
//       for (auto i = str.begin(); i != str.end(); ++i){
//         std::cout << *i;
//           }
//   // }
// }

// template <> void Print_String_Helper<ConsString>(ConsString* source, int depth) {
//   if (depth >= 5) {
//     std::cout << "ConsString Maximum Depth Reached! ";
//     return;
//   }else
//   {
//     StringShape shape(source);
//     DCHECK(shape.IsCons());
//     ++depth;
//       std::cout << "{";
//       tainttracking::Print_String_Helper(source->first(), depth);
//       tainttracking::Print_String_Helper(source->second(), depth);
//       std::cout << "} ";
//   }
// }

void OnNewDeserializedString(String* source) {
  MarkNewString(source);
  OnNewStringLiteral(source);/*
  int64_t ttaint = source->taint_info();
  uint8_t taint = static_cast<uint8_t>(ttaint);*/
  auto stype = GetTaintStatusRange(source, 0, source->length());
  if (FLAG_taint_flow_path_enable_cout == true && stype != TaintType::UNTAINTED) {
    std::cout << "NewDeserializedString " << stype <<"|";
    source->Print();
    std::cout << std::endl;
  }
  
}

template <class T, class S>
void OnNewSubStringCopy(T* source, S* dest, int offset, int length) {
  FlattenTaint(source, dest, offset, length);
  if (FLAG_taint_flow_path_enable_cout == true) {
    auto stype = GetTaintStatusRange(source, 0, source->length());
    auto dtype = GetTaintStatusRange(dest, 0, dest->length());
    /*int64_t sttaint = source->taint_info();
    uint8_t staint = static_cast<uint8_t>(sttaint);
    int64_t dttaint = dest->taint_info();
    uint8_t dtaint = static_cast<uint8_t>(dttaint);
    std::cout << "OnNewSubStringCopy_out from " << static_cast<TaintType>(staint & TaintType::TAINT_TYPE_MASK) <<"|";
    source->Print();
    std::cout << " to " << static_cast<TaintType>(dtaint & TaintType::TAINT_TYPE_MASK) <<"|";
    dest->Print();
    std::cout<<"\n"<<std::endl;*/
    if (stype != TaintType::UNTAINTED || dtype != TaintType::UNTAINTED) {
      std::cout << "OnNewSubStringCopy to " << dtype <<"|"; 
      dest->Print();
      std::cout << " from " << stype <<"|";
      source->Print();
      std::cout << std::endl;
      std::cout << " offset " << offset << " length " << length << std::endl;
    }
  }

  if (FLAG_taint_tracking_enable_symbolic) {
    LogSymbolic<1>(dest, {{source}}, std::to_string(offset), SLICE);
  }
}

void OnNewSlicedString(SlicedString* target, String* first,
                       int offset, int length) {
  MarkNewString(target);
  if (FLAG_taint_flow_path_enable_cout == true) {
  /*
  int64_t sttaint = target->taint_info();
  uint8_t staint = static_cast<uint8_t>(sttaint);
  int64_t dttaint = first->taint_info();
  uint8_t dtaint = static_cast<uint8_t>(dttaint);
  std::cout << "OnNewSlicedString_out target " << static_cast<TaintType>(staint & TaintType::TAINT_TYPE_MASK) <<"|";
  target->Print();
  std::cout << " first " << static_cast<TaintType>(dtaint & TaintType::TAINT_TYPE_MASK) <<"|";
  first->Print();
  std::cout<<"\n"<<std::endl;
  */
    auto ttype = GetTaintStatusRange(target, 0, target->length());
    auto ftype = GetTaintStatusRange(first, 0, first->length());
    if (ttype != TaintType::UNTAINTED || ftype != TaintType::UNTAINTED) {
      std::cout << "OnNewSlicedString to " << ttype <<"|";
      target->Print();
      std::cout << " from " << ftype <<"|";
      first->Print();
      std::cout << std::endl;
      std::cout << " offset " << offset << " length " << length << std::endl;
    }
  }
  
  if (FLAG_taint_tracking_enable_symbolic) {
    LogSymbolic<1>(target, {{first}}, std::to_string(offset), SLICE);
  }
}

template <class T, class S, class R>
void OnNewConcatStringCopy(T* dest, S* first, R* second) {

  TaintData* staint = StringTaintData<T>(dest); //dest->taint_info();
  if (FLAG_taint_flow_path_enable_cout == true) {
    /*int64_t fttaint = first->taint_info();
    uint8_t ftaint = static_cast<uint8_t>(fttaint);
    int64_t dttaint = second->taint_info();
    uint8_t dtaint = static_cast<uint8_t>(dttaint);
    std::cout<< "OnNewConcatStringCopy1_out"<<" first " << static_cast<TaintType>(ftaint & TaintType::TAINT_TYPE_MASK) << "|";
    first->Print();
    std::cout << " second " << static_cast<TaintType>(dtaint & TaintType::TAINT_TYPE_MASK) << "|";
    second->Print();
    std::cout<<"\n"<<std::endl;*/
    auto dtype = GetTaintStatusRange(dest, 0, dest->length());
    auto ftype = GetTaintStatusRange(first, 0, first->length());
    auto stype = GetTaintStatusRange(second, 0, second->length());
      if (dtype != TaintType::UNTAINTED || ftype != TaintType::UNTAINTED) {
      std::cout<< "OnNewConcatStringCopy to " << dtype << "|";
      dest->Print();
      std::cout << " from " << ftype << "|"; 
      first->Print();
      std::cout << std::endl << "OnNewConcatStringCopy to " << dtype << "|";
      dest->Print();
      std::cout << " from " << stype << "|";
      second->Print();
      std::cout << std::endl;
    }
  }
  
  if (FLAG_taint_tracking_enable_symbolic) {
    LogSymbolic<2>(dest, {{first, second}}, "", CONCAT);
  }
}

void OnNewConsString(ConsString* target, String* first, String* second) {
  MarkNewString(target);
  if (FLAG_taint_flow_path_enable_cout == true) {
    /*
    int64_t sttaint = target->taint_info();
    uint8_t staint = static_cast<uint8_t>(sttaint);
    int64_t fttaint = first->taint_info();
    uint8_t ftaint = static_cast<uint8_t>(fttaint);
    int64_t dttaint = second->taint_info();
    uint8_t dtaint = static_cast<uint8_t>(dttaint);
    std::cout << "NewConsString_out target " << static_cast<TaintType>(staint & TaintType::TAINT_TYPE_MASK) << "|";
    target->Print();
    std::cout << " first " << static_cast<TaintType>(ftaint & TaintType::TAINT_TYPE_MASK) << "|";
    first->Print();
    std::cout << " second " << static_cast<TaintType>(dtaint & TaintType::TAINT_TYPE_MASK) << "|";
    second->Print();
    std::cout<<"\n"<<std::endl;
    */
    auto ttype = GetTaintStatusRange(target, 0, target->length());
    auto ftype = GetTaintStatusRange(first, 0, first->length());
    auto stype = GetTaintStatusRange(second, 0, second->length());
    if (ttype != TaintType::UNTAINTED || ftype != TaintType::UNTAINTED || stype != TaintType::UNTAINTED) {
      std::cout << "NewConsString to " << ttype << "|";
      target->Print();
      std::cout << " from " << ftype << "|";
      first->Print();
      std::cout << std::endl << "NewConsString to " << ttype << "|";
      target->Print();
      std::cout << " from " << stype << "|";
      second->Print();
      std::cout << std::endl; 
    }
  }
  if (FLAG_taint_tracking_enable_symbolic) {
    LogSymbolic<2>(target, {{first, second}}, "", CONCAT);
  }
}

void OnNewFromJsonString(SeqString* target, String* source) {
  if (FLAG_taint_tracking_enable_symbolic) {
    LogSymbolic<1>(target, {{source}}, "", PARSED_JSON);
  }
  if (FLAG_taint_flow_path_enable_cout == true) {
    /*
    int64_t sttaint = target->taint_info();
    uint8_t staint = static_cast<uint8_t>(sttaint);
    int64_t dttaint = source->taint_info();
    uint8_t dtaint = static_cast<uint8_t>(dttaint);
    std::cout << "OnNewFromJsonString_out target " << static_cast<TaintType>(staint & TaintType::TAINT_TYPE_MASK) <<"|";
    target->Print();
    std::cout << " source " << static_cast<TaintType>(dtaint & TaintType::TAINT_TYPE_MASK) <<"|";
    source->Print();
    std::cout << std::endl;*/
    auto ttype = GetTaintStatusRange(target, 0, target->length());
    auto stype = GetTaintStatusRange(source, 0, source->length());
    if (ttype != TaintType::UNTAINTED || stype != TaintType::UNTAINTED) {
      std::cout << "OnNewFromJsonString to " << ttype <<"|";
      target->Print();
      std::cout << " from " << stype <<"|";
      source->Print();
      std::cout << std::endl;
    }
  }
}

template <class T> void OnNewExternalString(T* str) {
  MarkNewString(str);
  OnNewStringLiteral(str);
  if (FLAG_taint_flow_path_enable_cout == true) {
    /*
    int64_t ttaint = str->taint_info();
    uint8_t taint = static_cast<uint8_t>(ttaint);
    */
    auto ttype = GetTaintStatusRange(str, 0, str->length());
    if (ttype != TaintType::UNTAINTED) {
      std::cout << "NewExternalString: " << ttype << "|";
      str->Print();
      std::cout << std::endl;
    }
  }
}
template void OnNewExternalString<ExternalOneByteString>(
    ExternalOneByteString*);
template void OnNewExternalString<ExternalTwoByteString>(
    ExternalTwoByteString*);
/*
const char* ToCString(const String &value) {
  return *value ? *value : "<string conversion failed>";
}*/

template <class T>
void OnNewReplaceRegexpWithString(
    String* subject, T* result, JSRegExp* pattern, String* replacement) {
  if (FLAG_taint_tracking_enable_symbolic) {
    LogSymbolic<2>(result,
                   {{subject, String::cast(pattern->source())}},
                   replacement->ToCString().get(),
                   REGEXP);
  }
  if (FLAG_taint_flow_path_enable_cout == true) {
    /*
    int64_t sttaint = subject->taint_info();
    uint8_t staint = static_cast<uint8_t>(sttaint);
    int64_t dttaint = result->taint_info();
    uint8_t dtaint = static_cast<uint8_t>(dttaint);
    */
    // String *pat = pattern->Pattern();
    //const char* patt = ToCString(*pat);
    auto stype = GetTaintStatusRange(subject, 0, subject->length());
    auto rtype = GetTaintStatusRange(result, 0, result->length());
    std::cout << "OnNewReplaceRegexpWithString_out to " << rtype <<"|";
    result->Print();
    std::cout << " from " << stype <<"|";
    subject->Print();
    std::cout << std::endl;
    // std::cout << " RegExp1 " << pattern->DataAt(0)->Print() << " replacement " << replacement->ToCString().get() << std::endl;
    std::cout << " RegExp1 ";
    String::cast(pattern->DataAt(JSRegExp::kAtomPatternIndex))->Print();
    std::cout << " replacement " << replacement->ToCString().get() << std::endl;
    // String::cast(pattern_regexp->DataAt(JSRegExp::kAtomPatternIndex))
    // Vector<const uint8_t> each = pat->GetCharVector();
    // std::cout << " RegExp1 ";
    // for(int i = 0; i < each.length(); i ++) {
    //   std::cout << each[i];
    // }
    //PrintUC16(std::ostream &os, int start=0, int end=-1);
    // std::cout << " replacement " << std::endl;
    //printf(" RegExp1: %s replacement \n", pat->c_str());
    // if (stype != TaintType::UNTAINTED || rtype != TaintType::UNTAINTED) {
    //   std::cout << "OnNewReplaceRegexpWithString to " << rtype <<"|";
    //   result->Print();
    //   std::cout << " from " << stype <<"|";
    //   subject->Print();
    //   std::cout << std::endl;
    //   //std::cout << " RegExp " << pat << " replacement " << replacement->ToCString().get() << std::endl;
    // }
  }
}


template <class T, class Array>
void OnJoinManyStrings(T* target, Array* array) {
  if (FLAG_taint_tracking_enable_symbolic) {
    LogSymbolic<0>(target, {{}}, "TODO: print array value", JOIN);
  }
  if (FLAG_taint_flow_path_enable_cout == true) {
    /*
    int64_t ttaint = target->taint_info();
    uint8_t taint = static_cast<uint8_t>(ttaint);
    std::cout << "OnJoinManyStrings: " << static_cast<TaintType>(taint & TaintType::TAINT_TYPE_MASK) << "|";
    target->Print();
    std::cout << std::endl;
    */
    auto ttype = GetTaintStatusRange(target, 0, target->length());
    /*std::cout << "OnJoinManyStrings_out to "<< ttype << "|";
    target->Print();
    std::cout << " from " << 4 << "|";
    array[0].Print();
    
    for(int i = 0; i < len; i++) {
      std::cout << std::endl <<" Number " << i;
      array->get(i)->Print();
    }*/
    //std::cout << std::endl;
    if ( ttype != TaintType::UNTAINTED) {
      std::cout << "OnJoinManyStrings to "<< ttype << "|";
      target->Print();
      std::cout << " from " << ttype << "|";
      array[0].Print();
      std::cout << std::endl;
    }
  }
}

template <class T>
void OnConvertCase(String* source, T* answer) {
  FlattenTaint(source, answer, 0, source->length());
  if (FLAG_taint_flow_path_enable_cout == true) {
    /*
    int64_t sttaint = source->taint_info();
    uint8_t staint = static_cast<uint8_t>(sttaint);
    int64_t dttaint = answer->taint_info();
    uint8_t dtaint = static_cast<uint8_t>(dttaint);
    std::cout << "ConvertCase_out from " << static_cast<TaintType>(staint & TaintType::TAINT_TYPE_MASK) << "|";
    source->Print();
    std::cout << " to " << static_cast<TaintType>(dtaint & TaintType::TAINT_TYPE_MASK) << "|";
    answer->Print();
    std::cout << std::endl;
    */
    auto stype = GetTaintStatusRange(source, 0, source->length());
    auto atype = GetTaintStatusRange(answer, 0, answer->length());
    if (stype != TaintType::UNTAINTED || atype != TaintType::UNTAINTED) {
      std::cout << "ConvertCase to " << atype << "|";
      answer->Print();
      std::cout << " from " << stype << "|";
      source->Print();
      std::cout << std::endl;
    }
  }
  if (FLAG_taint_tracking_enable_symbolic) {
    LogSymbolic<1>(answer, {{source}}, "", CASE_CHANGE);
  }
}
template void OnConvertCase<SeqOneByteString>(
    String* source, SeqOneByteString* answer);
template void OnConvertCase<SeqTwoByteString>(
    String* source, SeqTwoByteString* answer);
template void OnConvertCase<SeqString>(
    String* source, SeqString* answer);

template void OnGenericOperation<String>(SymbolicType, String*);
template void OnGenericOperation<SeqOneByteString>(
    SymbolicType, SeqOneByteString*);
template void OnGenericOperation<SeqTwoByteString>(
    SymbolicType, SeqTwoByteString*);
template <class T>
void OnGenericOperation(SymbolicType type, T* source) {
  if (FLAG_taint_tracking_enable_symbolic) {
    LogSymbolic<0>(source, {{}}, "", type);
  }

  uint8_t anti_mask;
  uint8_t mask;
  switch (type) {
    case SymbolicType::URI_DECODE:
      anti_mask = TaintType::URL_ENCODED;
      mask = TaintType::URL_DECODED;
      break;

    case SymbolicType::URI_COMPONENT_DECODE:
      anti_mask = TaintType::URL_COMPONENT_ENCODED;
      mask = TaintType::URL_COMPONENT_DECODED;
      break;

    case SymbolicType::URI_UNESCAPE:
      anti_mask = TaintType::ESCAPE_ENCODED;
      mask = TaintType::ESCAPE_DECODED;
      break;

    case SymbolicType::URI_ENCODE:
      mask = TaintType::URL_ENCODED;
      anti_mask = TaintType::URL_DECODED;
      break;

    case SymbolicType::URI_COMPONENT_ENCODE:
      mask = TaintType::URL_COMPONENT_ENCODED;
      anti_mask = TaintType::URL_COMPONENT_DECODED;
      break;

    case SymbolicType::URI_ESCAPE:
      mask = TaintType::ESCAPE_ENCODED;
      anti_mask = TaintType::ESCAPE_DECODED;
      break;

    default:
      return;
  }

  // The encoding operations are required to return a flat string.
  DCHECK(source->IsSeqString());

  {
    DisallowHeapAllocation no_gc;
    SeqString* as_seq_ptr = SeqString::cast(source);

    int length = as_seq_ptr->length();
    TaintData type_arr [length];
    CopyOut(as_seq_ptr, type_arr, 0, length);

    for (int i = 0; i < length; i++) {
      uint8_t type_i = static_cast<uint8_t>(type_arr[i]);
      uint8_t old_encoding = type_i & TaintType::ENCODING_TYPE_MASK;

      // If the old encoding is nothing, then we move to the mask encoding. If the
      // old encoding was the inverse operation, then we move to no encoding. If
      // it is neither, then we move to the multiple encoding state.
      uint8_t new_encoding = old_encoding == TaintType::NO_ENCODING
        ? mask : (
            old_encoding == anti_mask
            ? TaintType::NO_ENCODING
            : TaintType::MULTIPLE_ENCODINGS);

      type_arr[i] = static_cast<TaintType>(
          (type_i & TaintType::TAINT_TYPE_MASK) | new_encoding);
    }

    // TODO: Perform this operation in-place without the copy in and copy out
    // calls.
    CopyIn(as_seq_ptr, type_arr, 0, length);
  }
}

void InsertControlFlowHook(ParseInfo* info) {
  DCHECK_NOT_NULL(info->literal());
  if (FLAG_taint_tracking_enable_export_ast ||
      FLAG_taint_tracking_enable_ast_modification ||
      FLAG_taint_tracking_enable_source_export ||
      FLAG_taint_tracking_enable_source_hash_export) {
    CHECK(SerializeAst(info));
  }
}

ConcolicExecutor& TaintTracker::Impl::Exec() {
  return exec_;
}

ObjectVersioner& TaintTracker::Impl::Versioner() {
  return *versioner_;
}


void LogRuntimeSymbolic(Isolate* isolate,
                        Handle<Object> target_object,
                        Handle<Object> label,
                        CheckType check) {
  MessageHolder message;
  auto log_message = message.InitRoot();
  auto cntrl_flow = log_message.getMessage().initRuntimeLog();
  BuilderSerializer serializer_out;
  V8NodeLabelSerializer serializer_in(isolate);
  NodeLabel out;
  CHECK_EQ(Status::OK, serializer_in.Deserialize(label, &out));
  CHECK_EQ(Status::OK, serializer_out.Serialize(cntrl_flow.initLabel(), out));
  bool isstring = target_object->IsString();
  if (isstring) {
    cntrl_flow.setObjectLabel(
        Handle<String>::cast(target_object)->taint_info());
  }
  switch (check) {
    case CheckType::STATEMENT_BEFORE:
      cntrl_flow.setCheckType(
          ::Ast::RuntimeLog::CheckType::STATEMENT_BEFORE);
      break;
    case CheckType::STATEMENT_AFTER:
      cntrl_flow.setCheckType(::Ast::RuntimeLog::CheckType::STATEMENT_AFTER);
      break;
    case CheckType::EXPRESSION_BEFORE:
      cntrl_flow.setCheckType(
          ::Ast::RuntimeLog::CheckType::EXPRESSION_BEFORE);
      break;
    case CheckType::EXPRESSION_AFTER:
    case CheckType::STATIC_VALUE_CHECK:
      cntrl_flow.setCheckType(
          ::Ast::RuntimeLog::CheckType::EXPRESSION_AFTER);
      break;
    default:
      UNREACHABLE();
  }

  TaintTracker::Impl::LogToFile(isolate, message);
}


uint64_t MAGIC_NUMBER = 0xbaededfeed;

V8NodeLabelSerializer::V8NodeLabelSerializer(Isolate* isolate) :
  isolate_(isolate) {};

Status V8NodeLabelSerializer::Serialize(
    Object** output, const NodeLabel& label) {
  if (!label.IsValid()) {
    return Status::FAILURE;
  }
  *output = *Make(label);
  return Status::OK;
}

v8::internal::Handle<v8::internal::Object> V8NodeLabelSerializer::Make(
    const NodeLabel& label) {
  auto* factory = isolate_->factory();
  Handle<SeqOneByteString> str = factory->NewRawOneByteString(
      sizeof(NodeLabel::Rand) +
      sizeof(NodeLabel::Counter) +
      sizeof(uint64_t)).ToHandleChecked();
  NodeLabel::Rand rand_val = label.GetRand();
  NodeLabel::Counter counter_val = label.GetCounter();
  MemCopy(str->GetChars(),
          reinterpret_cast<const uint8_t*>(&rand_val),
          sizeof(NodeLabel::Rand));
  MemCopy(str->GetChars() + sizeof(NodeLabel::Rand),
          reinterpret_cast<const uint8_t*>(&counter_val),
          sizeof(NodeLabel::Counter));
  MemCopy(str->GetChars() +
            sizeof(NodeLabel::Rand) +
            sizeof(NodeLabel::Counter),
          reinterpret_cast<const uint8_t*>(&MAGIC_NUMBER),
          sizeof(uint64_t));
  return str;
}

Status V8NodeLabelSerializer::Serialize(
    Handle<Object>* output, const NodeLabel& label) {
  if (!label.IsValid()) {
    return Status::FAILURE;
  }

  *output = Make(label);
  return Status::OK;
}

Status V8NodeLabelSerializer::Deserialize(
    Handle<Object> arr, NodeLabel* label) {
  DisallowHeapAllocation no_gc;
  return Deserialize(*arr, label);
}

Status V8NodeLabelSerializer::Deserialize(Object* arr, NodeLabel* label) {
  DisallowHeapAllocation no_gc;
  SeqOneByteString* seqstr = SeqOneByteString::cast(arr);
  if (!arr->IsSeqOneByteString()) {
    return Status::FAILURE;
  }
  NodeLabel::Rand rand_val;
  NodeLabel::Counter counter_val;
  MemCopy(reinterpret_cast<uint8_t*>(&rand_val),
          seqstr->GetChars(),
          sizeof(NodeLabel::Rand));
  MemCopy(reinterpret_cast<uint8_t*>(&counter_val),
          seqstr->GetChars() + sizeof(NodeLabel::Rand),
          sizeof(NodeLabel::Counter));
  if (sizeof(NodeLabel::Rand) +
      sizeof(NodeLabel::Counter) +
      sizeof(uint64_t) != seqstr->length()) {
    return Status::FAILURE;
  }
  uint64_t magic_number_check;
  MemCopy(reinterpret_cast<uint8_t*>(&magic_number_check),
          seqstr->GetChars() +
            sizeof(NodeLabel::Counter) +
            sizeof(NodeLabel::Rand),
          sizeof(uint64_t));
  if (magic_number_check != MAGIC_NUMBER) {
    return Status::FAILURE;
  }
  label->CopyFrom(NodeLabel(rand_val, counter_val));
  return label->IsValid() ? Status::OK : Status::FAILURE;
}


void RuntimeHook(Isolate* isolate,
                 Handle<Object> target_object,
                 Handle<Object> label,
                 int checktype) {
  DCHECK(FLAG_taint_tracking_enable_ast_modification);
  CheckType check = static_cast<CheckType>(checktype);


  if (FLAG_taint_tracking_enable_symbolic) {
    LogRuntimeSymbolic(
        isolate, target_object, label, check);
  }
  if (EnableConcolic()) {
    TaintTracker::FromIsolate(isolate)->Get()->Exec().OnRuntimeHook(
        target_object, label, check);
  }
}


void RuntimeHookVariableLoad(Isolate* isolate,
                             Handle<Object> target_object,
                             Handle<Object> proxy_label,
                             Handle<Object> past_assignment_label,
                             int checktype) {
  DCHECK(FLAG_taint_tracking_enable_ast_modification);
  CheckType check = static_cast<CheckType>(checktype);

  if (FLAG_taint_tracking_enable_symbolic) {
    LogRuntimeSymbolic(
        isolate, target_object, proxy_label, check);
  }
  if (EnableConcolic()) {
    TaintTracker::FromIsolate(isolate)->
      Get()->
      Exec().
      OnRuntimeHookVariableLoad(
          target_object, proxy_label, past_assignment_label, check);
  }
}

Handle<Object> RuntimeHookVariableStore(
    Isolate* isolate,
    Handle<Object> concrete,
    Handle<Object> label,
    CheckType checktype,
    Handle<Object> var_idx) {
  if (EnableConcolic()) {
    return TaintTracker::FromIsolate(isolate)->Get()->Exec().
      OnRuntimeHookVariableStore(concrete, label, checktype, var_idx);
  } else {
    return handle(isolate->heap()->undefined_value(), isolate);
  }
}

void RuntimeHookVariableContextStore(
    v8::internal::Isolate* isolate,
    v8::internal::Handle<v8::internal::Object> concrete,
    v8::internal::Handle<v8::internal::Object> label,
    v8::internal::Handle<v8::internal::Context> context,
    v8::internal::Handle<v8::internal::Smi> smi) {
  if (EnableConcolic()) {
    return TaintTracker::FromIsolate(isolate)->Get()->Exec().
      OnRuntimeHookVariableContextStore(concrete, label, context, smi);
  }
}

void RuntimeExitSymbolicStackFrame(v8::internal::Isolate* isolate) {
  if (EnableConcolic()) {
    return TaintTracker::FromIsolate(isolate)->Get()->Exec().
      ExitSymbolicStackFrame();
  }
}

void RuntimePrepareSymbolicStackFrame(
    v8::internal::Isolate* isolate,
    FrameType type) {
  if (EnableConcolic()) {
    return TaintTracker::FromIsolate(isolate)->Get()->Exec().
      PrepareSymbolicStackFrame(type);
  }
}

void RuntimeEnterSymbolicStackFrame(v8::internal::Isolate* isolate) {
  if (EnableConcolic()) {
    return TaintTracker::FromIsolate(isolate)->Get()->Exec().
      EnterSymbolicStackFrame();
  }
}

void RuntimeAddArgumentToStackFrame(
    v8::internal::Isolate* isolate,
    v8::internal::MaybeHandle<v8::internal::Object> label) {
  if (EnableConcolic()) {
    TaintTracker::FromIsolate(isolate)->Get()->Exec().
      AddArgumentToFrame(label);
  }
}

void RuntimeAddLiteralArgumentToStackFrame(
    v8::internal::Isolate* isolate,
    v8::internal::Handle<v8::internal::Object> value) {
  if (EnableConcolic()) {
    TaintTracker::FromIsolate(isolate)->Get()->Exec().
      AddLiteralArgumentToFrame(value);
  }
}

v8::internal::Handle<v8::internal::Object> GetSymbolicArgument(
    v8::internal::Isolate* isolate, uint32_t i) {
  if (EnableConcolic()) {
    return TaintTracker::FromIsolate(isolate)->Get()->Exec().
      GetSymbolicArgumentObject(i);
  } else {
    return handle(isolate->heap()->undefined_value(), isolate);
  }
}


void LogHeartBeat(v8::internal::Isolate* isolate) {
  MessageHolder holder;
  auto builder = holder.InitRoot();
  auto message = builder.getMessage();
  auto job_id_message = message.initJobId();
  job_id_message.setJobId(FLAG_taint_tracking_job_id);
  job_id_message.setTimestampMillisSinceEpoch(
      static_cast<int64_t>(v8::base::OS::TimeCurrentMillis()));
  TaintTracker::Impl::LogToFile(isolate, holder, FlushConfig::FORCE_FLUSH);
}

void HeartBeatTask::Run() {
  LogHeartBeat(isolate_);
  StartTimer(isolate_);
}


bool HasLabel(v8::internal::Isolate* isolate, const NodeLabel& label) {
  if (EnableConcolic()) {
    return TaintTracker::FromIsolate(isolate)->Get()->Exec().HasLabel(label);
  } else {
    return false;
  }
}

bool SymbolicMatchesFunctionArgs(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  if (EnableConcolic()) {
    return TaintTracker::FromIsolate(
        reinterpret_cast<v8::internal::Isolate*>(info.GetIsolate()))
      ->Get()->Exec().MatchesArgs(info);
  } else {
    return true;
  }
}

void RuntimeSetReturnValue(
    v8::internal::Isolate* isolate,
    v8::internal::Handle<v8::internal::Object> value,
    v8::internal::MaybeHandle<v8::internal::Object> label) {
  if (EnableConcolic()) {
    TaintTracker::FromIsolate(isolate)
      ->Get()->Exec().OnRuntimeSetReturnValue(value, label);
  }
}

void RuntimeEnterTry(v8::internal::Isolate* isolate,
                     v8::internal::Handle<v8::internal::Object> label) {
  if (EnableConcolic()) {
    TaintTracker::FromIsolate(isolate)->Get()->Exec().OnRuntimeEnterTry(label);
  }
}

void RuntimeExitTry(v8::internal::Isolate* isolate,
                    v8::internal::Handle<v8::internal::Object> label) {
  if (EnableConcolic()) {
    TaintTracker::FromIsolate(isolate)->Get()->Exec().OnRuntimeExitTry(label);
  }
}

void RuntimeOnThrow(v8::internal::Isolate* isolate,
                    v8::internal::Handle<v8::internal::Object> exception,
                    bool is_rethrow) {
  if (EnableConcolic()) {
    TaintTracker::FromIsolate(isolate)->Get()->Exec().OnRuntimeThrow(
        exception, is_rethrow);
  }
}

void RuntimeOnCatch(v8::internal::Isolate* isolate,
                    v8::internal::Handle<v8::internal::Object> thrown_object,
                    v8::internal::Handle<v8::internal::Context> context) {
  if (EnableConcolic()) {
    TaintTracker::FromIsolate(isolate)->Get()->Exec()
      .OnRuntimeCatch(thrown_object, context);
  }
}

void RuntimeOnExitFinally(v8::internal::Isolate* isolate) {
  if (EnableConcolic()) {
    TaintTracker::FromIsolate(isolate)->Get()->Exec()
      .OnRuntimeExitFinally();
  }
}

void RuntimeSetReceiver(v8::internal::Isolate* isolate,
                        v8::internal::Handle<v8::internal::Object> value,
                        v8::internal::Handle<v8::internal::Object> label) {
  if (EnableConcolic()) {
    TaintTracker::FromIsolate(isolate)->Get()->Exec()
      .SetReceiverOnFrame(value, label);
  }
}


v8::internal::Object* RuntimePrepareApplyFrame(
    v8::internal::Isolate* isolate,
    v8::internal::Handle<v8::internal::Object> argument_list,
    v8::internal::Handle<v8::internal::Object> target_fn,
    v8::internal::Handle<v8::internal::Object> new_target,
    v8::internal::Handle<v8::internal::Object> this_argument,
    FrameType frame_type) {
  if (EnableConcolic()) {
    TaintTracker::FromIsolate(isolate)->Get()->Exec()
      .RuntimePrepareApplyFrame(
          argument_list, target_fn, new_target, this_argument, frame_type);
  }
  return isolate->heap()->undefined_value();
}

v8::internal::Object* RuntimePrepareCallFrame(
    v8::internal::Isolate* isolate,
    v8::internal::Handle<v8::internal::Object> target_fn,
    FrameType caller_frame_type,
    v8::internal::Handle<v8::internal::FixedArray> args) {
  if (EnableConcolic()) {
    TaintTracker::FromIsolate(isolate)->Get()->Exec()
      .RuntimePrepareCallFrame(
          target_fn, caller_frame_type, args);
  }
  return isolate->heap()->undefined_value();
}

v8::internal::Object* RuntimePrepareCallOrConstructFrame(
    v8::internal::Isolate* isolate,
    v8::internal::Handle<v8::internal::Object> target_fn,
    v8::internal::Handle<v8::internal::Object> new_target,
    v8::internal::Handle<v8::internal::FixedArray> args) {
  if (EnableConcolic()) {
    TaintTracker::FromIsolate(isolate)->Get()->Exec()
      .RuntimePrepareCallOrConstructFrame(
          target_fn, new_target, args);
  }
  return isolate->heap()->undefined_value();
}


void RuntimeSetLiteralReceiver(
    v8::internal::Isolate* isolate,
    v8::internal::Handle<v8::internal::Object> target_fn) {
  if (EnableConcolic()) {
    TaintTracker::FromIsolate(isolate)->Get()->Exec()
      .SetLiteralReceiverOnCurrentFrame(target_fn);
  }
}

void RuntimeCheckMessageOrigin(
    v8::internal::Isolate* isolate,
    v8::internal::Handle<v8::internal::Object> left,
    v8::internal::Handle<v8::internal::Object> right,
    v8::internal::Token::Value token) {

  if (!left->IsString() || !right->IsString()) {
    return;
  }

  Handle<String> left_as_str = Handle<String>::cast(left);
  Handle<String> right_as_str = Handle<String>::cast(right);

  IsTaintedVisitor left_visitor;
  {
    DisallowHeapAllocation no_gc;
    left_visitor.run(*left_as_str, 0, left_as_str->length());
  }

  IsTaintedVisitor right_visitor;
  {
    DisallowHeapAllocation no_gc;
    right_visitor.run(*right_as_str, 0, right_as_str->length());
  }

  bool left_has_key = false;

  TaintFlag origin_flag = AddFlag(
      kTaintFlagUntainted, TaintType::MESSAGE_ORIGIN);
  if (left_visitor.GetFlag() == origin_flag) {
    left_has_key = true;
  } else if (right_visitor.GetFlag() != origin_flag) {
    return;
  }

  if (left_has_key) {
    TaintTracker::FromIsolate(isolate)->Get()->PutCrossOriginMessageTable(
        isolate,
        left_as_str,
        right_as_str);
  } else {
    TaintTracker::FromIsolate(isolate)->Get()->PutCrossOriginMessageTable(
        isolate,
        right_as_str,
        left_as_str);
  }
}


v8::internal::MaybeHandle<FixedArray>
TaintTracker::Impl::GetCrossOriginMessageTable(
    v8::internal::Handle<v8::internal::String> ref) {
  Isolate* isolate = ref->GetIsolate();
  Object* val = cross_origin_message_table_->Lookup(
      isolate->factory()->NewNumberFromInt64(ref->taint_info()));
  if (val) {
    if (val->IsFixedArray()) {
      return Handle<FixedArray>(FixedArray::cast(val), isolate);

    }
  }
  return MaybeHandle<FixedArray>();
}


void TaintTracker::Impl::PutCrossOriginMessageTable(
    v8::internal::Isolate* isolate,
    v8::internal::Handle<v8::internal::String> origin_taint,
    v8::internal::Handle<v8::internal::String> compare) {
  Handle<FixedArray> value = isolate->factory()->NewFixedArray(2);
  value->set(0, *origin_taint);
  value->set(1, *compare);
  int64_t key = origin_taint->taint_info();
  if (key == 0 || key == kUndefinedInstanceCounter) {
    // This means the key was not initialized, but there is a check.
    return;
  }

  DCHECK_NE(key, kUndefinedInstanceCounter);
  DCHECK_NE(key, 0);

  Handle<ObjectHashTable> new_table = ObjectHashTable::Put(
      cross_origin_message_table_,
      isolate->factory()->NewNumberFromInt64(key),
      value);

  if (new_table.location() != cross_origin_message_table_.location()) {
    cross_origin_message_table_ = Handle<ObjectHashTable>::cast(
        isolate->global_handles()->Create(*new_table.location()));
  }
}

void RuntimeParameterToContextStorage(
    v8::internal::Isolate* isolate,
    int parameter_index,
    int context_slot_index,
    v8::internal::Handle<v8::internal::Context> context) {
  if (EnableConcolic()) {
    TaintTracker::FromIsolate(isolate)->Get()->Exec()
      .OnRuntimeParameterToContextStorage(
          parameter_index, context_slot_index, context);
  }
}






}

STATIC_ASSERT(tainttracking::TaintType::UNTAINTED == 0);
STATIC_ASSERT(sizeof(tainttracking::TaintFlag) * kBitsPerByte >=
              tainttracking::TaintType::MAX_TAINT_TYPE);
